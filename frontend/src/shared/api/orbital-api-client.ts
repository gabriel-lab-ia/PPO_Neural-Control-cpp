import {
  type ApiEnvelope,
  type ArtifactListDto,
  type BenchmarkDto,
  type BenchmarkListDto,
  type ConfigPresetsDto,
  type CreateJobRequest,
  type HealthDto,
  type JobDto,
  type ReplayPayloadDto,
  type RunDto,
  type RunListDto,
  type TelemetryListDto,
  type VersionDto,
} from "@/shared/api/generated/orbital-api";
import { requestJson } from "@/shared/api/http-client";

interface PaginationQuery {
  limit?: number;
  offset?: number;
}

interface ReplayQuery {
  startStep?: number;
  endStep?: number;
  downsample?: number;
}

function withQuery(path: string, params: Record<string, number | string | undefined>): string {
  const query = new URLSearchParams();
  for (const [key, value] of Object.entries(params)) {
    if (value === undefined) {
      continue;
    }
    query.set(key, String(value));
  }

  const encoded = query.toString();
  return encoded.length > 0 ? `${path}?${encoded}` : path;
}

export interface OrbitalApiClient {
  getHealth: () => Promise<ApiEnvelope<HealthDto>>;
  getVersion: () => Promise<ApiEnvelope<VersionDto>>;

  listRuns: (query?: PaginationQuery) => Promise<ApiEnvelope<RunListDto>>;
  getRun: (runId: string) => Promise<ApiEnvelope<RunDto>>;
  getRunSummary: (runId: string) => Promise<ApiEnvelope<Record<string, unknown>>>;
  listRunTelemetry: (runId: string, query?: PaginationQuery) => Promise<ApiEnvelope<TelemetryListDto>>;
  getRunTelemetryWindow: (runId: string, query?: ReplayQuery) => Promise<ApiEnvelope<TelemetryListDto>>;
  listRunEvents: (runId: string, query?: PaginationQuery) => Promise<ApiEnvelope<{ run_id: string; count: number; items: unknown[] }>>;
  listRunArtifacts: (runId: string) => Promise<ApiEnvelope<ArtifactListDto>>;
  getRunReplay: (runId: string, query?: ReplayQuery) => Promise<ApiEnvelope<ReplayPayloadDto>>;

  listBenchmarks: (query?: PaginationQuery) => Promise<ApiEnvelope<BenchmarkListDto>>;
  getBenchmark: (benchmarkId: string) => Promise<ApiEnvelope<BenchmarkDto>>;

  createTrainJob: (request?: CreateJobRequest) => Promise<ApiEnvelope<JobDto>>;
  createEvalJob: (request?: CreateJobRequest) => Promise<ApiEnvelope<JobDto>>;
  createBenchmarkJob: (request?: CreateJobRequest) => Promise<ApiEnvelope<JobDto>>;
  getJob: (jobId: string) => Promise<ApiEnvelope<JobDto>>;

  getConfigPresets: () => Promise<ApiEnvelope<ConfigPresetsDto>>;
}

export function createOrbitalApiClient(baseUrl: string): OrbitalApiClient {
  const prefix = baseUrl.endsWith("/") ? baseUrl.slice(0, -1) : baseUrl;
  const absolute = (path: string): string => `${prefix}${path}`;

  return {
    getHealth: () => requestJson<ApiEnvelope<HealthDto>>(absolute("/health")),
    getVersion: () => requestJson<ApiEnvelope<VersionDto>>(absolute("/version")),

    listRuns: (query) =>
      requestJson<ApiEnvelope<RunListDto>>(
        absolute(withQuery("/runs", { limit: query?.limit, offset: query?.offset })),
      ),
    getRun: (runId) => requestJson<ApiEnvelope<RunDto>>(absolute(`/runs/${encodeURIComponent(runId)}`)),
    getRunSummary: (runId) =>
      requestJson<ApiEnvelope<Record<string, unknown>>>(
        absolute(`/runs/${encodeURIComponent(runId)}/summary`),
      ),
    listRunTelemetry: (runId, query) =>
      requestJson<ApiEnvelope<TelemetryListDto>>(
        absolute(
          withQuery(`/runs/${encodeURIComponent(runId)}/telemetry`, {
            limit: query?.limit,
            offset: query?.offset,
          }),
        ),
      ),
    getRunTelemetryWindow: (runId, query) =>
      requestJson<ApiEnvelope<TelemetryListDto>>(
        absolute(
          withQuery(`/runs/${encodeURIComponent(runId)}/telemetry/window`, {
            start_step: query?.startStep,
            end_step: query?.endStep,
            downsample: query?.downsample,
          }),
        ),
      ),
    listRunEvents: (runId, query) =>
      requestJson<ApiEnvelope<{ run_id: string; count: number; items: unknown[] }>>(
        absolute(
          withQuery(`/runs/${encodeURIComponent(runId)}/events`, {
            limit: query?.limit,
            offset: query?.offset,
          }),
        ),
      ),
    listRunArtifacts: (runId) =>
      requestJson<ApiEnvelope<ArtifactListDto>>(absolute(`/runs/${encodeURIComponent(runId)}/artifacts`)),
    getRunReplay: (runId, query) =>
      requestJson<ApiEnvelope<ReplayPayloadDto>>(
        absolute(
          withQuery(`/runs/${encodeURIComponent(runId)}/replay`, {
            start_step: query?.startStep,
            end_step: query?.endStep,
            downsample: query?.downsample,
          }),
        ),
      ),

    listBenchmarks: (query) =>
      requestJson<ApiEnvelope<BenchmarkListDto>>(
        absolute(withQuery("/benchmarks", { limit: query?.limit, offset: query?.offset })),
      ),
    getBenchmark: (benchmarkId) =>
      requestJson<ApiEnvelope<BenchmarkDto>>(absolute(`/benchmarks/${encodeURIComponent(benchmarkId)}`)),

    createTrainJob: (request) =>
      requestJson<ApiEnvelope<JobDto>>(absolute("/train/jobs"), {
        method: "POST",
        body: request ? JSON.stringify(request) : undefined,
      }),
    createEvalJob: (request) =>
      requestJson<ApiEnvelope<JobDto>>(absolute("/eval/jobs"), {
        method: "POST",
        body: request ? JSON.stringify(request) : undefined,
      }),
    createBenchmarkJob: (request) =>
      requestJson<ApiEnvelope<JobDto>>(absolute("/benchmark/jobs"), {
        method: "POST",
        body: request ? JSON.stringify(request) : undefined,
      }),
    getJob: (jobId) => requestJson<ApiEnvelope<JobDto>>(absolute(`/jobs/${encodeURIComponent(jobId)}`)),

    getConfigPresets: () => requestJson<ApiEnvelope<ConfigPresetsDto>>(absolute("/config/presets")),
  };
}
