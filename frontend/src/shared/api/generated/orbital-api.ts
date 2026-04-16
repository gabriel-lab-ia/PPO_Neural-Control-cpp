/*
 * Generated contract surface from docs/openapi/orbital-api.yaml
 * Source of truth: OpenAPI schema. Keep this file in sync when contracts evolve.
 */

export interface ApiEnvelope<TData> {
  status: "ok" | "error";
  timestamp: string;
  data: TData;
}

export interface ApiErrorEnvelope {
  status: "error";
  timestamp: string;
  error: {
    code: string;
    message: string;
    path: string;
  };
}

export interface RunDto {
  run_id: string;
  mode: string;
  environment: string;
  seed: number;
  started_at: string;
  ended_at: string | null;
  status: string;
  artifact_dir: string;
  config: Record<string, unknown> | null;
  summary: Record<string, unknown> | null;
}

export interface RunListDto {
  limit: number;
  offset: number;
  count: number;
  items: RunDto[];
}

export interface TelemetrySampleDto {
  step: number;
  mission_time_s: number;
  reward: number;
  action: number;
  value: number;
  control_magnitude: number;
  orbital_error_km: number;
  velocity_magnitude_kmps: number;
  policy_std: number;
  position_km: [number, number, number];
  velocity_kmps: [number, number, number];
  control_vector: [number, number, number];
  terminated: boolean;
  truncated: boolean;
  timestamp: string;
}

export interface TelemetryListDto {
  run_id: string;
  limit: number;
  offset: number;
  count: number;
  items: TelemetrySampleDto[];
}

export interface EventDto {
  id: number;
  run_id: string;
  level: string;
  event_type: string;
  message: string;
  payload: Record<string, unknown> | null;
  created_at: string;
}

export interface EventListDto {
  run_id: string;
  count: number;
  items: EventDto[];
}

export interface ArtifactDto {
  name: string;
  path: string;
  type: string;
  size_bytes: number;
}

export interface ArtifactListDto {
  run_id: string;
  count: number;
  items: ArtifactDto[];
}

export interface BenchmarkDto {
  benchmark_id: number;
  benchmark_name: string;
  run_id: string | null;
  summary: Record<string, unknown> | null;
  created_at: string;
}

export interface BenchmarkListDto {
  limit: number;
  offset: number;
  count: number;
  items: BenchmarkDto[];
}

export interface ReplayPayloadDto {
  run_id: string;
  run: RunDto | null;
  telemetry: TelemetrySampleDto[];
  events: EventDto[];
}

export interface JobDto {
  job_id: string;
  job_type: "train" | "eval" | "benchmark";
  status: "queued" | "running" | "completed" | "failed";
  run_id: string;
  created_at: string;
  updated_at: string;
  details: Record<string, unknown> | null;
}

export interface CreateJobRequest {
  run_id?: string;
  seed?: number;
  quick?: boolean;
}

export interface ConfigPresetDto {
  name: string;
  mode: string;
  description: string;
  arguments: Record<string, unknown>;
}

export interface ConfigPresetsDto {
  presets: ConfigPresetDto[];
}

export interface VersionDto {
  service: string;
  version: string;
  schema_version: string;
}

export interface HealthDto {
  service: string;
  status: string;
}

export interface WsEnvelopeBase<TPayload> {
  type: string;
  schema_version: string;
  timestamp: string;
  source: string;
  run_id: string;
  payload: TPayload;
}

export type WsTelemetrySample = WsEnvelopeBase<TelemetrySampleDto> & {
  type: "telemetry.sample";
};

export type WsReplayChunk = WsEnvelopeBase<{
  chunk_index: number;
  total_chunks: number;
  samples: TelemetrySampleDto[];
}> & {
  type: "replay.chunk";
};

export type WsMissionEvent = WsEnvelopeBase<Record<string, unknown>> & {
  type: "mission.event";
};

export type WsMessage = WsTelemetrySample | WsReplayChunk | WsMissionEvent;
