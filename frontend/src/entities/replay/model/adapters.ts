import type { EpisodeSummary } from "@/entities/episode/model/types";
import type { OrbitPathPoint } from "@/entities/orbit/model/types";
import type { ReplayFrame, ReplayRunDataset } from "@/entities/replay/model/types";
import type { BenchmarkSummary, MissionEventSummary, RunSummary } from "@/entities/run/model/types";
import type { TelemetrySample } from "@/entities/telemetry/model/types";
import type { ReplayPayloadDto, RunDto, TelemetrySampleDto } from "@/shared/api/generated/orbital-api";

function mapRunStatus(status: string): RunSummary["status"] {
  if (status === "completed") {
    return "ok";
  }
  if (status === "running") {
    return "running";
  }
  if (status === "failed") {
    return "failed";
  }
  return "warning";
}

function mapArtifactStatus(status: string): RunSummary["artifactStatus"] {
  if (status === "completed") {
    return "complete";
  }
  if (status === "running") {
    return "partial";
  }
  return "unknown";
}

function toTelemetry(runId: string, environment: string, sample: TelemetrySampleDto): TelemetrySample {
  return {
    runId,
    environment,
    timestep: sample.step,
    missionTimeS: sample.mission_time_s,
    reward: sample.reward,
    controlMagnitude: sample.control_magnitude,
    orbitalErrorKm: sample.orbital_error_km,
    velocityMagnitudeKmS: sample.velocity_magnitude_kmps,
    policyStd: sample.policy_std,
    backend: "libtorch_cpu",
    deterministic: true,
    positionKm: sample.position_km,
    velocityKmS: sample.velocity_kmps,
    controlVector: sample.control_vector,
    terminated: sample.terminated,
    truncated: sample.truncated,
    timestampIso: sample.timestamp,
  };
}

function toOrbit(sample: TelemetrySampleDto): OrbitPathPoint {
  return {
    timestep: sample.step,
    missionTimeS: sample.mission_time_s,
    positionKm: sample.position_km,
    velocityKmS: sample.velocity_kmps,
    orbitalErrorKm: sample.orbital_error_km,
    controlMagnitude: sample.control_magnitude,
    reward: sample.reward,
  };
}

function toFrame(runId: string, environment: string, index: number, sample: TelemetrySampleDto): ReplayFrame {
  return {
    frameIndex: index,
    timestampIso: sample.timestamp,
    telemetry: toTelemetry(runId, environment, sample),
    orbit: toOrbit(sample),
  };
}

function toEpisodes(telemetry: TelemetrySampleDto[]): EpisodeSummary[] {
  if (telemetry.length === 0) {
    return [];
  }

  const cumulativeReward = telemetry.reduce((accumulator, sample) => accumulator + sample.reward, 0);
  return [
    {
      episodeId: "episode-000",
      stepCount: telemetry.length,
      cumulativeReward,
      terminalReason: telemetry[telemetry.length - 1]?.terminated ? "goal" : "timeout",
    },
  ];
}

function toBenchmark(telemetry: TelemetrySampleDto[]): BenchmarkSummary {
  const meanReward = telemetry.length > 0
    ? telemetry.reduce((accumulator, sample) => accumulator + sample.reward, 0) / telemetry.length
    : 0;

  return {
    totalTimesteps: telemetry.length,
    meanReward,
    deterministic: true,
    backend: "libtorch_cpu",
    artifactStatus: telemetry.length > 0 ? "complete" : "partial",
  };
}

function toRunSummary(runId: string, run: RunDto | null, telemetry: TelemetrySampleDto[]): RunSummary {
  return {
    runId,
    label: runId,
    mode: run?.mode ?? "replay",
    environment: run?.environment ?? "point_mass",
    backend: "libtorch_cpu",
    deterministic: true,
    totalTimesteps: telemetry.length,
    status: mapRunStatus(run?.status ?? "running"),
    artifactStatus: mapArtifactStatus(run?.status ?? "running"),
    startedAtIso: run?.started_at ?? telemetry[0]?.timestamp ?? new Date().toISOString(),
  };
}

function toEvents(payload: ReplayPayloadDto): MissionEventSummary[] {
  return payload.events.map((event) => ({
    id: event.id,
    eventType: event.event_type,
    level: event.level,
    message: event.message,
    timestampIso: event.created_at,
  }));
}

export function replayPayloadToDataset(payload: ReplayPayloadDto): ReplayRunDataset {
  const runId = payload.run_id;
  const environment = payload.run?.environment ?? "point_mass";

  const frames = payload.telemetry.map((sample, index) => toFrame(runId, environment, index, sample));
  const orbitPath = frames.map((frame) => frame.orbit);

  return {
    run: toRunSummary(runId, payload.run, payload.telemetry),
    benchmark: toBenchmark(payload.telemetry),
    episodes: toEpisodes(payload.telemetry),
    events: toEvents(payload),
    frames,
    orbitPath,
  };
}
