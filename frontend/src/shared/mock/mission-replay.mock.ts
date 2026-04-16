import type { EpisodeSummary } from "@/entities/episode/model/types";
import type { OrbitPathPoint, Vector3 } from "@/entities/orbit/model/types";
import type { ReplayFrame, ReplayRunDataset } from "@/entities/replay/model/types";
import type { BenchmarkSummary, MissionEventSummary, RunSummary } from "@/entities/run/model/types";
import type { TelemetrySample } from "@/entities/telemetry/model/types";
import { DEFAULT_ENVIRONMENT } from "@/shared/config/replay";
import { magnitude3 } from "@/shared/lib/math";

function toIsoFromStep(step: number, offsetMinutes: number): string {
  const base = new Date("2026-04-01T12:00:00.000Z").getTime();
  return new Date(base + offsetMinutes * 60_000 + step * 1_000).toISOString();
}

function createFrame(runId: string, step: number, phase: number): ReplayFrame {
  const theta = step * 0.045 + phase;
  const radiusKm = 6_800 + 45 * Math.sin(step * 0.018 + phase * 1.1);
  const positionKm: Vector3 = [
    radiusKm * Math.cos(theta),
    radiusKm * Math.sin(theta),
    250 * Math.sin(theta * 0.5 + phase),
  ];
  const velocityKmS: Vector3 = [
    -7.6 * Math.sin(theta),
    7.6 * Math.cos(theta),
    0.38 * Math.cos(theta * 0.5 + phase),
  ];

  const orbitalErrorKm = radiusKm - 6_800;
  const controlMagnitude =
    0.12 +
    0.03 * Math.sin(step * 0.13 + phase * 0.7) +
    0.02 * Math.cos(step * 0.041 + phase);
  const reward =
    1.2 -
    Math.abs(orbitalErrorKm) * 0.0042 -
    controlMagnitude * 0.45 +
    0.03 * Math.sin(step * 0.08 + phase);

  const telemetry: TelemetrySample = {
    runId,
    environment: DEFAULT_ENVIRONMENT,
    timestep: step,
    missionTimeS: step,
    reward,
    controlMagnitude,
    orbitalErrorKm,
    velocityMagnitudeKmS: magnitude3(velocityKmS),
    policyStd: 0.17 + 0.015 * Math.cos(step * 0.06 + phase),
    backend: "libtorch_cpu",
    deterministic: true,
    positionKm,
    velocityKmS,
    controlVector: [
      controlMagnitude * Math.cos(theta),
      controlMagnitude * Math.sin(theta),
      0.5 * controlMagnitude * Math.sin(theta * 0.35),
    ],
  };

  const orbit: OrbitPathPoint = {
    timestep: step,
    missionTimeS: step,
    positionKm,
    velocityKmS,
    orbitalErrorKm,
    controlMagnitude,
    reward,
  };

  return {
    frameIndex: step,
    timestampIso: toIsoFromStep(step, Math.round(phase * 10)),
    orbit,
    telemetry,
  };
}

function createEpisodes(frames: number): EpisodeSummary[] {
  const chunk = Math.floor(frames / 3);
  return [
    {
      episodeId: "ep-001",
      stepCount: chunk,
      cumulativeReward: chunk * 1.06,
      terminalReason: "timeout",
    },
    {
      episodeId: "ep-002",
      stepCount: chunk,
      cumulativeReward: chunk * 1.09,
      terminalReason: "goal",
    },
    {
      episodeId: "ep-003",
      stepCount: frames - chunk * 2,
      cumulativeReward: (frames - chunk * 2) * 1.03,
      terminalReason: "timeout",
    },
  ];
}

function createDataset(runId: string, label: string, phase: number): ReplayRunDataset {
  const frameCount = 360;
  const frames = Array.from({ length: frameCount }, (_, step) => createFrame(runId, step, phase));

  const run: RunSummary = {
    runId,
    label,
    mode: "benchmark",
    environment: DEFAULT_ENVIRONMENT,
    backend: "libtorch_cpu",
    deterministic: true,
    totalTimesteps: frameCount,
    status: "ok",
    artifactStatus: "complete",
    startedAtIso: toIsoFromStep(0, Math.round(phase * 10)),
  };

  const meanReward =
    frames.reduce((accumulator, frame) => accumulator + frame.telemetry.reward, 0) / frames.length;

  const benchmark: BenchmarkSummary = {
    totalTimesteps: frameCount,
    meanReward,
    deterministic: true,
    backend: "libtorch_cpu",
    artifactStatus: "complete",
  };

  const events: MissionEventSummary[] = [
    {
      id: 1,
      eventType: "run.started",
      level: "info",
      message: "Replay dataset bootstrapped.",
      timestampIso: frames[0]?.timestampIso ?? new Date().toISOString(),
    },
    {
      id: 2,
      eventType: "benchmark.completed",
      level: "info",
      message: "Deterministic smoke benchmark completed.",
      timestampIso: frames[Math.max(0, frames.length - 1)]?.timestampIso ?? new Date().toISOString(),
    },
  ];

  return {
    run,
    benchmark,
    episodes: createEpisodes(frameCount),
    events,
    frames,
    orbitPath: frames.map((frame) => frame.orbit),
  };
}

export const MISSION_REPLAY_DATASETS: ReplayRunDataset[] = [
  createDataset("smoke_local", "Smoke Benchmark Local", 0.2),
  createDataset("train_cpu_001", "Training Baseline CPU", 0.95),
  createDataset("eval_cpu_001", "Evaluation Deterministic", 1.5),
];

export const MISSION_REPLAY_DATASET_BY_ID = new Map(
  MISSION_REPLAY_DATASETS.map((dataset) => [dataset.run.runId, dataset] as const),
);
