import type { EpisodeSummary } from "@/entities/episode/model/types";
import type { OrbitPathPoint } from "@/entities/orbit/model/types";
import type { BenchmarkSummary, MissionEventSummary, RunSummary } from "@/entities/run/model/types";
import type { TelemetrySample } from "@/entities/telemetry/model/types";

export interface ReplayFrame {
  frameIndex: number;
  timestampIso: string;
  orbit: OrbitPathPoint;
  telemetry: TelemetrySample;
}

export interface ReplayRunDataset {
  run: RunSummary;
  benchmark: BenchmarkSummary;
  episodes: EpisodeSummary[];
  events: MissionEventSummary[];
  frames: ReplayFrame[];
  orbitPath: OrbitPathPoint[];
}
