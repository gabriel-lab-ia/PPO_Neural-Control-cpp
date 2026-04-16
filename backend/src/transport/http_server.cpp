#include "transport/http_server.h"

#include "common/http_utils.h"
#include "common/json.h"
#include "common/logger.h"
#include "common/time_utils.h"
#include "transport/json_serialization.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

namespace orbital::backend::transport {
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

namespace {

std::string extract_json_string(std::string_view body, const std::string& key) {
    const std::string body_string(body);
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(body_string, match, pattern) && match.size() > 1) {
        return match[1].str();
    }
    return {};
}

std::optional<std::int64_t> extract_json_int(std::string_view body, const std::string& key) {
    const std::string body_string(body);
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (std::regex_search(body_string, match, pattern) && match.size() > 1) {
        try {
            return std::stoll(match[1].str());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<bool> extract_json_bool(std::string_view body, const std::string& key) {
    const std::string body_string(body);
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(body_string, match, pattern) && match.size() > 1) {
        const std::string value = match[1].str();
        return value == "true";
    }
    return std::nullopt;
}

void send_response(
    tcp::socket& socket,
    const http::status status,
    std::string body,
    const std::string_view service_name,
    const std::string& content_type = "application/json"
) {
    http::response<http::string_body> response{status, 11};
    response.set(http::field::server, service_name);
    response.set(http::field::content_type, content_type);
    response.keep_alive(false);
    response.body() = std::move(body);
    response.prepare_payload();
    http::write(socket, response);
}

std::string telemetry_chunk_payload(
    const std::vector<domain::TelemetrySample>& samples,
    const std::size_t start,
    const std::size_t end,
    const std::size_t chunk_index,
    const std::size_t total_chunks
) {
    std::ostringstream stream;
    stream << '{'
           << "\"chunk_index\":" << chunk_index << ','
           << "\"total_chunks\":" << total_chunks << ','
           << "\"samples\":[";

    for (std::size_t i = start; i < end; ++i) {
        if (i > start) {
            stream << ',';
        }
        stream << telemetry_sample_to_json(samples[i]);
    }

    stream << "]}";
    return stream.str();
}

void stream_live_telemetry(
    websocket::stream<tcp::socket>& ws,
    const ServerConfig& config,
    application::MissionService& mission_service,
    const std::string& run_id
) {
    const auto samples = mission_service.get_telemetry_window(run_id, 0, -1, 1200);
    if (samples.empty()) {
        ws.text(true);
        ws.write(boost::asio::buffer(common::ws_envelope(
            "mission.event",
            config.schema_version,
            common::now_utc_iso8601(),
            config.source,
            run_id,
            R"({"event":"stream.empty"})"
        )));
        return;
    }

    for (const auto& sample : samples) {
        ws.text(true);
        ws.write(boost::asio::buffer(common::ws_envelope(
            "telemetry.sample",
            config.schema_version,
            common::now_utc_iso8601(),
            config.source,
            run_id,
            telemetry_sample_to_json(sample)
        )));
        std::this_thread::sleep_for(std::chrono::milliseconds(75));
    }

    ws.text(true);
    ws.write(boost::asio::buffer(common::ws_envelope(
        "mission.event",
        config.schema_version,
        common::now_utc_iso8601(),
        config.source,
        run_id,
        R"({"event":"stream.completed"})"
    )));
}

void stream_replay_chunks(
    websocket::stream<tcp::socket>& ws,
    const ServerConfig& config,
    application::MissionService& mission_service,
    const std::string& run_id
) {
    const auto replay_window = mission_service.build_replay({
        .run_id = run_id,
        .start_step = 0,
        .end_step = -1,
        .downsample_points = 1200,
        .event_limit = 256,
    });

    constexpr std::size_t kChunkSize = 120;
    const std::size_t total_chunks = replay_window.samples.empty()
        ? 0
        : (replay_window.samples.size() + kChunkSize - 1U) / kChunkSize;

    for (std::size_t chunk_index = 0; chunk_index < total_chunks; ++chunk_index) {
        const std::size_t start = chunk_index * kChunkSize;
        const std::size_t end = std::min(replay_window.samples.size(), start + kChunkSize);

        ws.text(true);
        ws.write(boost::asio::buffer(common::ws_envelope(
            "replay.chunk",
            config.schema_version,
            common::now_utc_iso8601(),
            config.source,
            run_id,
            telemetry_chunk_payload(replay_window.samples, start, end, chunk_index + 1U, total_chunks)
        )));
        std::this_thread::sleep_for(std::chrono::milliseconds(45));
    }

    ws.text(true);
    ws.write(boost::asio::buffer(common::ws_envelope(
        "mission.event",
        config.schema_version,
        common::now_utc_iso8601(),
        config.source,
        run_id,
        R"({"event":"replay.completed"})"
    )));
}

void handle_websocket(
    tcp::socket socket,
    http::request<http::string_body> request,
    const ServerConfig& config,
    application::MissionService& mission_service
) {
    websocket::stream<tcp::socket> ws(std::move(socket));
    ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws.accept(request);

    const auto target = common::parse_target(std::string(request.target()));
    const auto segments = common::split_path(target.path);

    if (target.path == "/ws/telemetry/live") {
        const std::string run_id = [&]() {
            const auto runs = mission_service.list_runs(1, 0);
            if (!runs.empty()) {
                return runs.front().run_id;
            }
            return std::string("latest");
        }();
        stream_live_telemetry(ws, config, mission_service, run_id);
    } else if (segments.size() == 4U && segments[0] == "ws" && segments[1] == "runs" && segments[3] == "stream") {
        stream_replay_chunks(ws, config, mission_service, segments[2]);
    } else {
        ws.text(true);
        ws.write(boost::asio::buffer(common::ws_envelope(
            "mission.event",
            config.schema_version,
            common::now_utc_iso8601(),
            config.source,
            "",
            R"({"event":"ws.route_not_found"})"
        )));
    }

    ws.close(websocket::close_code::normal);
}

std::string version_payload(const ServerConfig& config) {
    std::ostringstream stream;
    stream << '{'
           << "\"service\":" << common::json_string(config.service_name) << ','
           << "\"version\":" << common::json_string(config.service_version) << ','
           << "\"schema_version\":" << common::json_string(config.schema_version)
           << '}';
    return stream.str();
}

http::status handle_get(
    const common::RequestTarget& target,
    const ServerConfig& config,
    application::MissionService& mission_service,
    application::JobService& job_service,
    std::string& response_json
) {
    if (target.path == "/health") {
        response_json = ok_response("{\"service\":\"orbital-backend\",\"status\":\"healthy\"}");
        return http::status::ok;
    }

    if (target.path == "/version") {
        response_json = ok_response(version_payload(config));
        return http::status::ok;
    }

    if (target.path == "/runs") {
        const std::int64_t limit = std::clamp(
            common::parse_int_or(target.query.contains("limit") ? target.query.at("limit") : "", 50),
            static_cast<std::int64_t>(1),
            static_cast<std::int64_t>(500)
        );
        const std::int64_t offset = std::max<std::int64_t>(0, common::parse_int_or(target.query.contains("offset") ? target.query.at("offset") : "", 0));
        const auto runs = mission_service.list_runs(limit, offset);
        response_json = ok_response(runs_to_json(runs, limit, offset));
        return http::status::ok;
    }

    if (target.path == "/benchmarks") {
        const std::int64_t limit = std::clamp(
            common::parse_int_or(target.query.contains("limit") ? target.query.at("limit") : "", 50),
            static_cast<std::int64_t>(1),
            static_cast<std::int64_t>(500)
        );
        const std::int64_t offset = std::max<std::int64_t>(0, common::parse_int_or(target.query.contains("offset") ? target.query.at("offset") : "", 0));
        const auto benchmarks = mission_service.list_benchmarks(limit, offset);
        response_json = ok_response(benchmarks_to_json(benchmarks, limit, offset));
        return http::status::ok;
    }

    if (target.path == "/config/presets") {
        response_json = ok_response(config_presets_json());
        return http::status::ok;
    }

    const auto segments = common::split_path(target.path);
    if (segments.size() == 2U && segments[0] == "benchmarks") {
        const auto benchmark = mission_service.get_benchmark(segments[1]);
        if (!benchmark.has_value()) {
            response_json = error_response("not_found", "benchmark not found", target.path);
            return http::status::not_found;
        }

        response_json = ok_response(benchmark_to_json(*benchmark));
        return http::status::ok;
    }

    if (segments.size() == 2U && segments[0] == "jobs") {
        const auto job = job_service.get(segments[1]);
        if (!job.has_value()) {
            response_json = error_response("not_found", "job not found", target.path);
            return http::status::not_found;
        }

        response_json = ok_response(job_to_json(*job));
        return http::status::ok;
    }

    if (segments.size() >= 2U && segments[0] == "runs") {
        const std::string run_id = segments[1];

        if (segments.size() == 2U) {
            const auto run = mission_service.get_run(run_id);
            if (!run.has_value()) {
                response_json = error_response("not_found", "run not found", target.path);
                return http::status::not_found;
            }

            response_json = ok_response(run_to_json(*run));
            return http::status::ok;
        }

        if (segments.size() == 3U && segments[2] == "summary") {
            const auto run = mission_service.get_run(run_id);
            if (!run.has_value()) {
                response_json = error_response("not_found", "run not found", target.path);
                return http::status::not_found;
            }

            response_json = ok_response(run->summary_json.empty() ? "{}" : run->summary_json);
            return http::status::ok;
        }

        if (segments.size() == 3U && segments[2] == "telemetry") {
            const std::int64_t limit = std::clamp(
                common::parse_int_or(target.query.contains("limit") ? target.query.at("limit") : "", 500),
                static_cast<std::int64_t>(1),
                static_cast<std::int64_t>(5000)
            );
            const std::int64_t offset = std::max<std::int64_t>(0, common::parse_int_or(target.query.contains("offset") ? target.query.at("offset") : "", 0));

            const auto telemetry = mission_service.get_telemetry(run_id, limit, offset);
            response_json = ok_response(telemetry_to_json(telemetry, run_id, limit, offset));
            return http::status::ok;
        }

        if (segments.size() == 4U && segments[2] == "telemetry" && segments[3] == "window") {
            const std::int64_t start_step = std::max<std::int64_t>(0, common::parse_int_or(target.query.contains("start_step") ? target.query.at("start_step") : "", 0));
            const std::int64_t end_step = common::parse_int_or(target.query.contains("end_step") ? target.query.at("end_step") : "", -1);
            const std::int64_t downsample_points = std::clamp(
                common::parse_int_or(target.query.contains("downsample") ? target.query.at("downsample") : "", 1200),
                static_cast<std::int64_t>(1),
                static_cast<std::int64_t>(10000)
            );

            const auto telemetry = mission_service.get_telemetry_window(run_id, start_step, end_step, downsample_points);
            response_json = ok_response(telemetry_to_json(telemetry, run_id, downsample_points, 0));
            return http::status::ok;
        }

        if (segments.size() == 3U && segments[2] == "events") {
            const std::int64_t limit = std::clamp(
                common::parse_int_or(target.query.contains("limit") ? target.query.at("limit") : "", 500),
                static_cast<std::int64_t>(1),
                static_cast<std::int64_t>(5000)
            );
            const std::int64_t offset = std::max<std::int64_t>(0, common::parse_int_or(target.query.contains("offset") ? target.query.at("offset") : "", 0));
            const auto events = mission_service.list_events(run_id, limit, offset);
            response_json = ok_response(events_to_json(events, run_id));
            return http::status::ok;
        }

        if (segments.size() == 3U && segments[2] == "artifacts") {
            const auto artifacts = mission_service.list_artifacts(run_id);
            response_json = ok_response(artifacts_to_json(artifacts, run_id));
            return http::status::ok;
        }

        if (segments.size() == 3U && segments[2] == "replay") {
            const std::int64_t start_step = std::max<std::int64_t>(0, common::parse_int_or(target.query.contains("start_step") ? target.query.at("start_step") : "", 0));
            const std::int64_t end_step = common::parse_int_or(target.query.contains("end_step") ? target.query.at("end_step") : "", -1);
            const std::int64_t downsample_points = std::clamp(
                common::parse_int_or(target.query.contains("downsample") ? target.query.at("downsample") : "", 1200),
                static_cast<std::int64_t>(1),
                static_cast<std::int64_t>(10000)
            );

            const auto replay_window = mission_service.build_replay({
                .run_id = run_id,
                .start_step = start_step,
                .end_step = end_step,
                .downsample_points = downsample_points,
                .event_limit = 512,
            });
            response_json = ok_response(replay_to_json(replay_window));
            return http::status::ok;
        }
    }

    response_json = error_response("not_found", "route not found", target.path);
    return http::status::not_found;
}

http::status handle_post(
    const common::RequestTarget& target,
    const std::string_view body,
    application::JobService& job_service,
    std::string& response_json
) {
    auto parse_job_request = [&](const domain::JobType type) {
        application::JobLaunchRequest request;
        request.type = type;
        request.run_id = extract_json_string(body, "run_id");
        request.seed = extract_json_int(body, "seed").value_or(7);
        request.quick = extract_json_bool(body, "quick").value_or(true);
        return request;
    };

    if (target.path == "/train/jobs") {
        const auto job = job_service.submit(parse_job_request(domain::JobType::Train));
        response_json = ok_response(job_to_json(job));
        return http::status::accepted;
    }

    if (target.path == "/eval/jobs") {
        const auto job = job_service.submit(parse_job_request(domain::JobType::Eval));
        response_json = ok_response(job_to_json(job));
        return http::status::accepted;
    }

    if (target.path == "/benchmark/jobs") {
        const auto job = job_service.submit(parse_job_request(domain::JobType::Benchmark));
        response_json = ok_response(job_to_json(job));
        return http::status::accepted;
    }

    response_json = error_response("not_found", "route not found", target.path);
    return http::status::not_found;
}

void handle_session(
    tcp::socket socket,
    const ServerConfig& config,
    application::MissionService& mission_service,
    application::JobService& job_service
) {
    beast::error_code error;
    beast::flat_buffer buffer;

    http::request<http::string_body> request;
    http::read(socket, buffer, request, error);
    if (error) {
        common::log(common::LogLevel::Warn, std::string("http read failed: ") + error.message());
        return;
    }

    if (websocket::is_upgrade(request)) {
        handle_websocket(std::move(socket), std::move(request), config, mission_service);
        return;
    }

    const auto target = common::parse_target(std::string(request.target()));

    try {
        std::string response_json;
        http::status status = http::status::internal_server_error;

        switch (request.method()) {
            case http::verb::get:
                status = handle_get(target, config, mission_service, job_service, response_json);
                break;
            case http::verb::post:
                status = handle_post(target, request.body(), job_service, response_json);
                break;
            default:
                status = http::status::method_not_allowed;
                response_json = error_response("method_not_allowed", "only GET and POST are supported", target.path);
                break;
        }

        send_response(socket, status, std::move(response_json), config.service_name);
    } catch (const std::exception& ex) {
        common::log(common::LogLevel::Error, std::string("session error: ") + ex.what());
        send_response(
            socket,
            http::status::internal_server_error,
            error_response("internal_error", ex.what(), target.path),
            config.service_name
        );
    }

    socket.shutdown(tcp::socket::shutdown_send, error);
}

}  // namespace

HttpServer::HttpServer(
    ServerConfig config,
    application::MissionService& mission_service,
    application::JobService& job_service
)
    : config_(std::move(config)),
      mission_service_(mission_service),
      job_service_(job_service) {}

void HttpServer::run() {
    boost::asio::io_context io_context{1};
    tcp::acceptor acceptor(io_context, {tcp::v4(), config_.port});

    common::log(
        common::LogLevel::Info,
        "orbital backend listening on 0.0.0.0:" + std::to_string(config_.port)
    );

    while (true) {
        beast::error_code error;
        tcp::socket socket(io_context);
        acceptor.accept(socket, error);
        if (error) {
            common::log(common::LogLevel::Warn, "accept failed: " + error.message());
            continue;
        }

        std::thread{
            handle_session,
            std::move(socket),
            config_,
            std::ref(mission_service_),
            std::ref(job_service_)
        }.detach();
    }
}

}  // namespace orbital::backend::transport
