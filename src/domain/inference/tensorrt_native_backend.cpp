#include "domain/inference/tensorrt_native_backend.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(NMC_WITH_TENSORRT)
#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>
#endif

namespace nmc::domain::inference {
namespace {

std::string precision_suffix(const InferencePrecision precision) {
    switch (precision) {
        case InferencePrecision::kFp16:
            return "fp16";
        case InferencePrecision::kInt8:
            return "int8";
        case InferencePrecision::kFp32:
            return "fp32";
    }
    return "fp32";
}

}  // namespace

std::filesystem::path default_engine_path_for_onnx(
    const std::filesystem::path& onnx_path,
    const InferencePrecision precision
) {
    const auto stem = onnx_path.stem().string();
    const auto suffix = precision_suffix(precision);
    return onnx_path.parent_path() / (stem + "." + suffix + ".engine");
}

#if defined(NMC_WITH_TENSORRT)
namespace {

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool has_engine_extension(const std::filesystem::path& path) {
    const auto extension = lower_copy(path.extension().string());
    return extension == ".engine" || extension == ".plan";
}

bool has_onnx_extension(const std::filesystem::path& path) {
    return lower_copy(path.extension().string()) == ".onnx";
}

template <typename T>
struct TrtDeleter {
    void operator()(T* ptr) const noexcept {
        if (ptr != nullptr) {
            ptr->destroy();
        }
    }
};

template <typename T>
using TrtUnique = std::unique_ptr<T, TrtDeleter<T>>;

std::size_t data_type_size(const nvinfer1::DataType data_type) {
    switch (data_type) {
        case nvinfer1::DataType::kFLOAT:
            return 4U;
        case nvinfer1::DataType::kHALF:
            return 2U;
        case nvinfer1::DataType::kINT8:
            return 1U;
        case nvinfer1::DataType::kINT32:
            return 4U;
        case nvinfer1::DataType::kBOOL:
            return 1U;
#if NV_TENSORRT_MAJOR >= 9
        case nvinfer1::DataType::kUINT8:
            return 1U;
#endif
    }
    throw std::runtime_error("unsupported TensorRT data type");
}

std::size_t dims_volume(const nvinfer1::Dims& dims) {
    std::size_t volume = 1U;
    for (int32_t i = 0; i < dims.nbDims; ++i) {
        const auto dim = dims.d[i];
        if (dim < 0) {
            throw std::runtime_error("dynamic dim reached volume computation without profile resolution");
        }
        volume *= static_cast<std::size_t>(dim);
    }
    return volume;
}

bool has_dynamic_dims(const nvinfer1::Dims& dims) {
    for (int32_t i = 0; i < dims.nbDims; ++i) {
        if (dims.d[i] < 0) {
            return true;
        }
    }
    return false;
}

nvinfer1::Dims make_fixed_input_dims(
    nvinfer1::Dims dims,
    const int32_t batch_size,
    const int32_t observation_dim
) {
    for (int32_t i = 0; i < dims.nbDims; ++i) {
        if (dims.d[i] >= 0) {
            continue;
        }
        if (i == 0) {
            dims.d[i] = batch_size;
        } else {
            dims.d[i] = observation_dim;
        }
    }
    return dims;
}

class TensorRtLogger final : public nvinfer1::ILogger {
public:
    explicit TensorRtLogger(Severity minimum_severity) : minimum_severity_(minimum_severity) {}

    void log(const Severity severity, const char* message) noexcept override {
        if (severity > minimum_severity_ || message == nullptr) {
            return;
        }
        std::cerr << "[TensorRT] " << message << '\n';
    }

private:
    Severity minimum_severity_ = Severity::kWARNING;
};

void check_cuda(const cudaError_t status, const std::string_view context) {
    if (status != cudaSuccess) {
        throw std::runtime_error(
            "CUDA error at " + std::string(context) + ": " + std::string(cudaGetErrorString(status))
        );
    }
}

class DeviceBuffer final {
public:
    DeviceBuffer() = default;
    ~DeviceBuffer() {
        reset();
    }

    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;

    DeviceBuffer(DeviceBuffer&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          capacity_bytes_(std::exchange(other.capacity_bytes_, 0U)) {}

    DeviceBuffer& operator=(DeviceBuffer&& other) noexcept {
        if (this != &other) {
            reset();
            data_ = std::exchange(other.data_, nullptr);
            capacity_bytes_ = std::exchange(other.capacity_bytes_, 0U);
        }
        return *this;
    }

    void reserve(const std::size_t bytes) {
        if (bytes <= capacity_bytes_) {
            return;
        }

        reset();
        void* new_ptr = nullptr;
        check_cuda(cudaMalloc(&new_ptr, bytes), "cudaMalloc");
        data_ = new_ptr;
        capacity_bytes_ = bytes;
    }

    [[nodiscard]] void* data() noexcept {
        return data_;
    }

    [[nodiscard]] const void* data() const noexcept {
        return data_;
    }

private:
    void reset() noexcept {
        if (data_ != nullptr) {
            cudaFree(data_);
            data_ = nullptr;
        }
        capacity_bytes_ = 0U;
    }

    void* data_ = nullptr;
    std::size_t capacity_bytes_ = 0U;
};

class CudaStream final {
public:
    CudaStream() {
        check_cuda(cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking), "cudaStreamCreateWithFlags");
    }

    ~CudaStream() {
        if (stream_ != nullptr) {
            cudaStreamDestroy(stream_);
            stream_ = nullptr;
        }
    }

    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;

    [[nodiscard]] cudaStream_t get() const noexcept {
        return stream_;
    }

private:
    cudaStream_t stream_ = nullptr;
};

class RandomInt8Calibrator final : public nvinfer1::IInt8EntropyCalibrator2 {
public:
    RandomInt8Calibrator(
        const int32_t batch_size,
        const int32_t observation_dim,
        const int32_t total_batches,
        const std::filesystem::path& calibration_cache_path,
        const uint64_t seed
    )
        : batch_size_(batch_size),
          observation_dim_(observation_dim),
          total_batches_(total_batches),
          calibration_cache_path_(calibration_cache_path),
          generator_(seed),
          distribution_(-1.0f, 1.0f) {
        const auto element_count = static_cast<std::size_t>(batch_size_) * static_cast<std::size_t>(observation_dim_);
        host_buffer_.resize(element_count);
        const auto bytes = element_count * sizeof(float);
        check_cuda(cudaMalloc(&device_buffer_, bytes), "calibrator cudaMalloc");
    }

    ~RandomInt8Calibrator() override {
        if (device_buffer_ != nullptr) {
            cudaFree(device_buffer_);
            device_buffer_ = nullptr;
        }
    }

    int getBatchSize() const noexcept override {
        return batch_size_;
    }

    bool getBatch(void* bindings[], const char* const*, const int32_t nb_bindings) noexcept override {
        if (bindings == nullptr || nb_bindings <= 0 || current_batch_ >= total_batches_) {
            return false;
        }

        try {
            for (auto& value : host_buffer_) {
                value = distribution_(generator_);
            }

            const auto bytes = host_buffer_.size() * sizeof(float);
            check_cuda(
                cudaMemcpy(device_buffer_, host_buffer_.data(), bytes, cudaMemcpyHostToDevice),
                "calibrator cudaMemcpy"
            );

            bindings[0] = device_buffer_;
            ++current_batch_;
            return true;
        } catch (...) {
            return false;
        }
    }

    const void* readCalibrationCache(std::size_t& length) noexcept override {
        cache_.clear();
        length = 0U;

        if (calibration_cache_path_.empty() || !std::filesystem::exists(calibration_cache_path_)) {
            return nullptr;
        }

        try {
            std::ifstream input(calibration_cache_path_, std::ios::binary);
            if (!input.good()) {
                return nullptr;
            }

            input.seekg(0, std::ios::end);
            const auto size = input.tellg();
            input.seekg(0, std::ios::beg);
            if (size <= 0) {
                return nullptr;
            }

            cache_.resize(static_cast<std::size_t>(size));
            input.read(reinterpret_cast<char*>(cache_.data()), size);
            length = cache_.size();
            return cache_.data();
        } catch (...) {
            cache_.clear();
            length = 0U;
            return nullptr;
        }
    }

    void writeCalibrationCache(const void* cache, const std::size_t length) noexcept override {
        if (calibration_cache_path_.empty() || cache == nullptr || length == 0U) {
            return;
        }

        try {
            std::filesystem::create_directories(calibration_cache_path_.parent_path());
            std::ofstream output(calibration_cache_path_, std::ios::binary | std::ios::trunc);
            output.write(reinterpret_cast<const char*>(cache), static_cast<std::streamsize>(length));
        } catch (...) {
            // Best effort only.
        }
    }

private:
    int32_t batch_size_ = 1;
    int32_t observation_dim_ = 1;
    int32_t total_batches_ = 1;
    std::filesystem::path calibration_cache_path_;
    int32_t current_batch_ = 0;
    std::vector<float> host_buffer_;
    void* device_buffer_ = nullptr;
    std::vector<uint8_t> cache_;
    std::mt19937 generator_;
    std::uniform_real_distribution<float> distribution_;
};

struct TensorBinding {
    std::string name;
    int32_t index = -1;
    bool is_input = false;
    nvinfer1::DataType data_type = nvinfer1::DataType::kFLOAT;
    nvinfer1::Dims max_dims{};
    DeviceBuffer device;
};

std::filesystem::path resolve_engine_path(
    const std::filesystem::path& model_path,
    const TensorRtBuildOptions& options
) {
    if (has_engine_extension(model_path)) {
        return model_path;
    }
    if (!has_onnx_extension(model_path)) {
        throw std::runtime_error("TensorRT native backend requires .onnx or .engine/.plan input");
    }

    const auto default_path = default_engine_path_for_onnx(model_path, options.requested_precision);
    if (!options.engine_cache_dir.empty()) {
        std::filesystem::create_directories(options.engine_cache_dir);
        return options.engine_cache_dir / default_path.filename();
    }
    return default_path;
}

InferencePrecision resolve_requested_precision(
    const InferencePrecision requested,
    const nvinfer1::IBuilder& builder
) {
    if (requested == InferencePrecision::kFp16) {
        return builder.platformHasFastFp16() ? InferencePrecision::kFp16 : InferencePrecision::kFp32;
    }

    if (requested == InferencePrecision::kInt8) {
        if (builder.platformHasFastInt8()) {
            return InferencePrecision::kInt8;
        }
        return builder.platformHasFastFp16() ? InferencePrecision::kFp16 : InferencePrecision::kFp32;
    }

    return InferencePrecision::kFp32;
}

torch::ScalarType to_torch_dtype(const nvinfer1::DataType data_type) {
    switch (data_type) {
        case nvinfer1::DataType::kFLOAT:
            return torch::kFloat32;
        case nvinfer1::DataType::kHALF:
            return torch::kFloat16;
        case nvinfer1::DataType::kINT8:
            return torch::kInt8;
        case nvinfer1::DataType::kINT32:
            return torch::kInt32;
        case nvinfer1::DataType::kBOOL:
            return torch::kBool;
#if NV_TENSORRT_MAJOR >= 9
        case nvinfer1::DataType::kUINT8:
            return torch::kUInt8;
#endif
    }
    return torch::kFloat32;
}

std::vector<int64_t> dims_to_shape(const nvinfer1::Dims& dims) {
    std::vector<int64_t> shape;
    shape.reserve(static_cast<std::size_t>(dims.nbDims));
    for (int32_t i = 0; i < dims.nbDims; ++i) {
        shape.push_back(static_cast<int64_t>(dims.d[i]));
    }
    return shape;
}

}  // namespace

struct TensorRtNativeBackend::Impl {
    Impl(int64_t observation_dim_value, int64_t action_dim_value, InferencePrecision requested_precision_value)
        : observation_dim(observation_dim_value),
          action_dim(action_dim_value),
          requested_precision(requested_precision_value),
          logger(nvinfer1::ILogger::Severity::kWARNING),
          stream() {}

    void load_from_onnx_or_engine(const std::filesystem::path& model_path, const TensorRtBuildOptions& options) {
        build_options = options;
        build_options.requested_precision = requested_precision;

        if (has_onnx_extension(model_path)) {
            const auto engine_path = resolve_engine_path(model_path, build_options);
            const bool can_load_cached = build_options.enable_engine_cache && !build_options.force_rebuild &&
                std::filesystem::exists(engine_path);
            if (can_load_cached) {
                load_engine_from_disk(engine_path);
                return;
            }
            build_engine_from_onnx(model_path, engine_path);
            return;
        }

        if (has_engine_extension(model_path)) {
            load_engine_from_disk(model_path);
            return;
        }

        throw std::runtime_error(
            "unsupported TensorRT model extension for native backend: " + model_path.string()
        );
    }

    InferenceOutput infer(const torch::Tensor& observation) {
        if (!loaded || context == nullptr || engine == nullptr) {
            throw std::runtime_error("TensorRT native backend not loaded");
        }

        torch::Tensor batch = observation;
        if (batch.dim() == 1) {
            batch = batch.unsqueeze(0);
        }

        batch = batch.to(torch::kCPU, torch::kFloat32).contiguous();
        if (batch.dim() != 2) {
            throw std::runtime_error("TensorRT backend expects observation tensor shape [batch, observation_dim]");
        }

        const auto batch_size = static_cast<int32_t>(batch.size(0));
        const auto obs_dim = static_cast<int32_t>(batch.size(1));
        if (obs_dim != static_cast<int32_t>(observation_dim)) {
            throw std::runtime_error(
                "observation dim mismatch for TensorRT backend: expected " +
                std::to_string(observation_dim) + ", got " + std::to_string(obs_dim)
            );
        }

        if (batch_size <= 0 || batch_size > build_options.max_batch_size) {
            throw std::runtime_error(
                "batch size " + std::to_string(batch_size) +
                " out of supported range [1," + std::to_string(build_options.max_batch_size) + "]"
            );
        }

        const auto input_dims = make_fixed_input_dims(
            engine->getBindingDimensions(input_binding),
            batch_size,
            static_cast<int32_t>(observation_dim)
        );
        if (!context->setBindingDimensions(input_binding, input_dims)) {
            throw std::runtime_error("failed to set TensorRT input binding dimensions");
        }
        if (!context->allInputDimensionsSpecified()) {
            throw std::runtime_error("TensorRT context input dimensions not fully specified");
        }

        const auto input_bytes = static_cast<std::size_t>(batch.numel()) * sizeof(float);
        check_cuda(
            cudaMemcpyAsync(
                bindings[static_cast<std::size_t>(input_binding)].device.data(),
                batch.data_ptr<float>(),
                input_bytes,
                cudaMemcpyHostToDevice,
                stream.get()
            ),
            "cudaMemcpyAsync input"
        );

        if (!context->enqueueV2(enqueue_bindings.data(), stream.get(), nullptr)) {
            throw std::runtime_error("TensorRT enqueueV2 failed");
        }

        auto action_dims = context->getBindingDimensions(action_binding);
        auto value_dims = context->getBindingDimensions(value_binding);

        if (has_dynamic_dims(action_dims) || has_dynamic_dims(value_dims)) {
            throw std::runtime_error("dynamic output dims unresolved after TensorRT execution");
        }

        auto action_raw = copy_output_to_tensor(action_binding, action_dims);
        auto value_raw = copy_output_to_tensor(value_binding, value_dims);

        check_cuda(cudaStreamSynchronize(stream.get()), "cudaStreamSynchronize");

        auto action = action_raw.to(torch::kFloat32);
        auto value = value_raw.to(torch::kFloat32);

        return {
            std::move(action),
            std::move(value)
        };
    }

    InferenceBackendCapabilities capabilities() const {
        return {
            .supports_dynamic_shapes = supports_dynamic_shapes,
            .supports_fp16 = supports_fp16,
            .supports_int8 = supports_int8,
            .uses_cuda = loaded,
            .is_emulated = false,
            .runtime = loaded ? "tensorrt_native_cuda" : "tensorrt_native_unloaded",
            .configured_precision = active_precision
        };
    }

    bool is_loaded() const noexcept {
        return loaded;
    }

    std::filesystem::path active_engine_path() const {
        return loaded_engine_path;
    }

private:
    void build_engine_from_onnx(const std::filesystem::path& onnx_path, const std::filesystem::path& engine_path) {
        TrtUnique<nvinfer1::IBuilder> builder(nvinfer1::createInferBuilder(logger));
        if (builder == nullptr) {
            throw std::runtime_error("failed to create TensorRT builder");
        }

        constexpr uint32_t explicit_batch_flag = 1U << static_cast<uint32_t>(
            nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH
        );
        TrtUnique<nvinfer1::INetworkDefinition> network(builder->createNetworkV2(explicit_batch_flag));
        if (network == nullptr) {
            throw std::runtime_error("failed to create TensorRT network");
        }

        TrtUnique<nvonnxparser::IParser> parser(nvonnxparser::createParser(*network, logger));
        if (parser == nullptr) {
            throw std::runtime_error("failed to create ONNX parser");
        }

        if (!parser->parseFromFile(onnx_path.string().c_str(), static_cast<int32_t>(nvinfer1::ILogger::Severity::kWARNING))) {
            std::string message = "TensorRT ONNX parse failed";
            for (int32_t i = 0; i < parser->getNbErrors(); ++i) {
                const auto* error = parser->getError(i);
                if (error != nullptr) {
                    message += "\n - ";
                    message += error->desc();
                }
            }
            throw std::runtime_error(message);
        }

        TrtUnique<nvinfer1::IBuilderConfig> config(builder->createBuilderConfig());
        if (config == nullptr) {
            throw std::runtime_error("failed to create TensorRT builder config");
        }

#if NV_TENSORRT_MAJOR >= 8
        config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, build_options.workspace_size_bytes);
#else
        config->setMaxWorkspaceSize(build_options.workspace_size_bytes);
#endif

        if (!build_options.allow_tf32) {
            config->clearFlag(nvinfer1::BuilderFlag::kTF32);
        }

        supports_fp16 = builder->platformHasFastFp16();
        supports_int8 = builder->platformHasFastInt8();
        active_precision = resolve_requested_precision(requested_precision, *builder);

        if (active_precision == InferencePrecision::kFp16 || active_precision == InferencePrecision::kInt8) {
            config->setFlag(nvinfer1::BuilderFlag::kFP16);
        }

        std::unique_ptr<RandomInt8Calibrator> calibrator;
        if (active_precision == InferencePrecision::kInt8) {
            config->setFlag(nvinfer1::BuilderFlag::kINT8);
            const auto calibration_cache = build_options.calibration_cache_path.empty()
                ? (engine_path.parent_path() / (engine_path.stem().string() + ".calib.cache"))
                : build_options.calibration_cache_path;
            calibrator = std::make_unique<RandomInt8Calibrator>(
                build_options.calibration_batch_size,
                static_cast<int32_t>(observation_dim),
                build_options.calibration_batches,
                calibration_cache,
                build_options.calibration_seed
            );
            config->setInt8Calibrator(calibrator.get());
        }

        supports_dynamic_shapes = false;
        TrtUnique<nvinfer1::IOptimizationProfile> profile(builder->createOptimizationProfile());
        if (profile == nullptr) {
            throw std::runtime_error("failed to create TensorRT optimization profile");
        }

        bool profile_required = false;
        for (int32_t i = 0; i < network->getNbInputs(); ++i) {
            auto* input = network->getInput(i);
            if (input == nullptr) {
                continue;
            }

            const auto raw_dims = input->getDimensions();
            if (!has_dynamic_dims(raw_dims)) {
                continue;
            }

            supports_dynamic_shapes = true;
            profile_required = true;

            auto min_dims = raw_dims;
            auto opt_dims = raw_dims;
            auto max_dims = raw_dims;
            for (int32_t axis = 0; axis < raw_dims.nbDims; ++axis) {
                if (raw_dims.d[axis] >= 0) {
                    continue;
                }

                if (axis == 0) {
                    min_dims.d[axis] = 1;
                    opt_dims.d[axis] = std::max(1, build_options.max_batch_size / 4);
                    max_dims.d[axis] = std::max(1, build_options.max_batch_size);
                } else {
                    const auto fixed_dim = (axis == raw_dims.nbDims - 1)
                        ? static_cast<int32_t>(observation_dim)
                        : 1;
                    min_dims.d[axis] = fixed_dim;
                    opt_dims.d[axis] = fixed_dim;
                    max_dims.d[axis] = fixed_dim;
                }
            }

            if (!profile->setDimensions(input->getName(), nvinfer1::OptProfileSelector::kMIN, min_dims) ||
                !profile->setDimensions(input->getName(), nvinfer1::OptProfileSelector::kOPT, opt_dims) ||
                !profile->setDimensions(input->getName(), nvinfer1::OptProfileSelector::kMAX, max_dims)) {
                throw std::runtime_error("failed to configure TensorRT optimization profile for input: " + std::string(input->getName()));
            }
        }

        if (profile_required) {
            if (!config->addOptimizationProfile(profile.get())) {
                throw std::runtime_error("failed to add TensorRT optimization profile");
            }
        }

        TrtUnique<nvinfer1::IHostMemory> serialized(builder->buildSerializedNetwork(*network, *config));
        if (serialized == nullptr) {
            throw std::runtime_error("TensorRT failed to build serialized engine from ONNX");
        }

        std::filesystem::create_directories(engine_path.parent_path());
        {
            std::ofstream output(engine_path, std::ios::binary | std::ios::trunc);
            if (!output.good()) {
                throw std::runtime_error("failed to open engine output file: " + engine_path.string());
            }
            output.write(
                static_cast<const char*>(serialized->data()),
                static_cast<std::streamsize>(serialized->size())
            );
        }

        deserialize_engine(serialized->data(), serialized->size(), engine_path);
    }

    void load_engine_from_disk(const std::filesystem::path& engine_path) {
        if (!std::filesystem::exists(engine_path)) {
            throw std::runtime_error("TensorRT engine not found: " + engine_path.string());
        }

        std::ifstream input(engine_path, std::ios::binary);
        if (!input.good()) {
            throw std::runtime_error("failed to read TensorRT engine: " + engine_path.string());
        }

        input.seekg(0, std::ios::end);
        const auto size = input.tellg();
        input.seekg(0, std::ios::beg);
        if (size <= 0) {
            throw std::runtime_error("TensorRT engine file is empty: " + engine_path.string());
        }

        std::vector<char> bytes(static_cast<std::size_t>(size));
        input.read(bytes.data(), size);
        deserialize_engine(bytes.data(), bytes.size(), engine_path);
    }

    void deserialize_engine(const void* data, const std::size_t size, const std::filesystem::path& source_path) {
        runtime.reset(nvinfer1::createInferRuntime(logger));
        if (runtime == nullptr) {
            throw std::runtime_error("failed to create TensorRT runtime");
        }

        engine.reset(runtime->deserializeCudaEngine(data, size));
        if (engine == nullptr) {
            throw std::runtime_error("failed to deserialize TensorRT engine");
        }

        context.reset(engine->createExecutionContext());
        if (context == nullptr) {
            throw std::runtime_error("failed to create TensorRT execution context");
        }

        loaded_engine_path = source_path;
        setup_bindings();
        loaded = true;
    }

    void setup_bindings() {
        const auto nb_bindings = engine->getNbBindings();
        if (nb_bindings <= 0) {
            throw std::runtime_error("TensorRT engine has no bindings");
        }

        bindings.clear();
        enqueue_bindings.clear();
        bindings.reserve(static_cast<std::size_t>(nb_bindings));
        enqueue_bindings.resize(static_cast<std::size_t>(nb_bindings), nullptr);

        input_binding = -1;
        action_binding = -1;
        value_binding = -1;
        std::vector<int32_t> output_candidates;

        for (int32_t i = 0; i < nb_bindings; ++i) {
            TensorBinding binding;
            binding.index = i;
            binding.name = engine->getBindingName(i);
            binding.is_input = engine->bindingIsInput(i);
            binding.data_type = engine->getBindingDataType(i);

            auto dims = engine->getBindingDimensions(i);
            if (has_dynamic_dims(dims)) {
                dims = engine->getProfileDimensions(i, 0, nvinfer1::OptProfileSelector::kMAX);
            }
            binding.max_dims = dims;

            const auto bytes = dims_volume(binding.max_dims) * data_type_size(binding.data_type);
            binding.device.reserve(bytes);
            enqueue_bindings[static_cast<std::size_t>(i)] = binding.device.data();

            if (binding.is_input) {
                if (input_binding >= 0) {
                    throw std::runtime_error("TensorRT backend currently supports a single input tensor");
                }
                input_binding = i;
            } else {
                output_candidates.push_back(i);
            }

            bindings.emplace_back(std::move(binding));
        }

        if (input_binding < 0) {
            throw std::runtime_error("failed to identify TensorRT input binding");
        }
        if (output_candidates.size() < 2U) {
            throw std::runtime_error("TensorRT engine must expose at least two outputs (action and value)");
        }

        auto classify_by_name = [&](const int32_t index) {
            const auto& name = bindings[static_cast<std::size_t>(index)].name;
            const auto normalized = lower_copy(name);
            if (action_binding < 0 && (normalized.find("action") != std::string::npos ||
                                       normalized.find("policy") != std::string::npos ||
                                       normalized.find("mean") != std::string::npos)) {
                action_binding = index;
            }
            if (value_binding < 0 && normalized.find("value") != std::string::npos) {
                value_binding = index;
            }
        };

        for (const auto index : output_candidates) {
            classify_by_name(index);
        }

        auto classify_by_dims = [&](const int32_t index) {
            const auto dims = bindings[static_cast<std::size_t>(index)].max_dims;
            if (dims.nbDims <= 0) {
                return;
            }

            const auto last_dim = dims.d[dims.nbDims - 1];
            if (last_dim == 1 && value_binding < 0) {
                value_binding = index;
            } else if (last_dim == static_cast<int32_t>(action_dim) && action_binding < 0) {
                action_binding = index;
            }
        };

        for (const auto index : output_candidates) {
            classify_by_dims(index);
        }

        if (action_binding < 0) {
            action_binding = output_candidates[0];
        }
        if (value_binding < 0) {
            value_binding = output_candidates[1];
        }

        if (action_binding == value_binding) {
            throw std::runtime_error("TensorRT output classification collapsed to one tensor");
        }
    }

    torch::Tensor copy_output_to_tensor(const int32_t binding_index, const nvinfer1::Dims& dims) {
        const auto& binding = bindings[static_cast<std::size_t>(binding_index)];
        const auto shape = dims_to_shape(dims);
        const auto num_elements = dims_volume(dims);
        const auto bytes = num_elements * data_type_size(binding.data_type);
        auto tensor = torch::empty(shape, torch::TensorOptions().dtype(to_torch_dtype(binding.data_type)).device(torch::kCPU));

        check_cuda(
            cudaMemcpyAsync(
                tensor.data_ptr(),
                binding.device.data(),
                bytes,
                cudaMemcpyDeviceToHost,
                stream.get()
            ),
            "cudaMemcpyAsync output"
        );
        return tensor;
    }

    int64_t observation_dim = 0;
    int64_t action_dim = 0;
    InferencePrecision requested_precision = InferencePrecision::kFp32;
    InferencePrecision active_precision = InferencePrecision::kFp32;

    bool loaded = false;
    bool supports_dynamic_shapes = false;
    bool supports_fp16 = false;
    bool supports_int8 = false;

    TensorRtBuildOptions build_options{};
    std::filesystem::path loaded_engine_path;

    TensorRtLogger logger;
    CudaStream stream;
    TrtUnique<nvinfer1::IRuntime> runtime{nullptr};
    TrtUnique<nvinfer1::ICudaEngine> engine{nullptr};
    TrtUnique<nvinfer1::IExecutionContext> context{nullptr};
    std::vector<TensorBinding> bindings;
    std::vector<void*> enqueue_bindings;

    int32_t input_binding = -1;
    int32_t action_binding = -1;
    int32_t value_binding = -1;
};

TensorRtNativeBackend::TensorRtNativeBackend(
    const int64_t observation_dim,
    const int64_t action_dim,
    const InferencePrecision requested_precision
)
    : impl_(std::make_unique<Impl>(observation_dim, action_dim, requested_precision)) {}

TensorRtNativeBackend::~TensorRtNativeBackend() = default;
TensorRtNativeBackend::TensorRtNativeBackend(TensorRtNativeBackend&&) noexcept = default;
TensorRtNativeBackend& TensorRtNativeBackend::operator=(TensorRtNativeBackend&&) noexcept = default;

TensorRtBuildOptions TensorRtNativeBackend::default_build_options() const {
    TensorRtBuildOptions options;
    options.requested_precision = impl_->requested_precision;
    options.max_batch_size = 16;
    options.enable_dynamic_shapes = true;
    options.workspace_size_bytes = (1ULL << 30U);
    options.allow_tf32 = true;
    options.force_rebuild = false;
    options.enable_engine_cache = true;
    options.calibration_batch_size = 8;
    options.calibration_batches = 32;
    options.calibration_seed = 7;
    return options;
}

void TensorRtNativeBackend::load_from_onnx_or_engine(
    const std::filesystem::path& model_path,
    const TensorRtBuildOptions& options
) {
    impl_->load_from_onnx_or_engine(model_path, options);
}

InferenceOutput TensorRtNativeBackend::infer(const torch::Tensor& observation, const bool) {
    return impl_->infer(observation);
}

InferenceBackendCapabilities TensorRtNativeBackend::capabilities() const {
    return impl_->capabilities();
}

bool TensorRtNativeBackend::is_loaded() const noexcept {
    return impl_->is_loaded();
}

std::filesystem::path TensorRtNativeBackend::active_engine_path() const {
    return impl_->active_engine_path();
}

#else

struct TensorRtNativeBackend::Impl {
    Impl(int64_t, int64_t, InferencePrecision requested_precision_value)
        : requested_precision(requested_precision_value) {}

    InferencePrecision requested_precision = InferencePrecision::kFp32;
    std::filesystem::path loaded_engine_path;
};

TensorRtNativeBackend::TensorRtNativeBackend(
    const int64_t observation_dim,
    const int64_t action_dim,
    const InferencePrecision requested_precision
)
    : impl_(std::make_unique<Impl>(observation_dim, action_dim, requested_precision)) {}

TensorRtNativeBackend::~TensorRtNativeBackend() = default;
TensorRtNativeBackend::TensorRtNativeBackend(TensorRtNativeBackend&&) noexcept = default;
TensorRtNativeBackend& TensorRtNativeBackend::operator=(TensorRtNativeBackend&&) noexcept = default;

TensorRtBuildOptions TensorRtNativeBackend::default_build_options() const {
    TensorRtBuildOptions options;
    options.requested_precision = impl_->requested_precision;
    return options;
}

void TensorRtNativeBackend::load_from_onnx_or_engine(
    const std::filesystem::path&,
    const TensorRtBuildOptions&
) {
    throw std::runtime_error(
        "TensorRT native backend is unavailable in this build. Reconfigure with NMC_ENABLE_TENSORRT=ON."
    );
}

InferenceOutput TensorRtNativeBackend::infer(const torch::Tensor&, const bool) {
    throw std::runtime_error(
        "TensorRT native backend is unavailable in this build. Reconfigure with NMC_ENABLE_TENSORRT=ON."
    );
}

InferenceBackendCapabilities TensorRtNativeBackend::capabilities() const {
    return {
        .supports_dynamic_shapes = false,
        .supports_fp16 = false,
        .supports_int8 = false,
        .uses_cuda = false,
        .is_emulated = true,
        .runtime = "tensorrt_unavailable",
        .configured_precision = impl_->requested_precision
    };
}

bool TensorRtNativeBackend::is_loaded() const noexcept {
    return false;
}

std::filesystem::path TensorRtNativeBackend::active_engine_path() const {
    return impl_->loaded_engine_path;
}

#endif

}  // namespace nmc::domain::inference
