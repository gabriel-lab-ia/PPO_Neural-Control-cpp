"use client";

import { useEffect, useMemo, useState } from "react";

import type { ReplayFrame, ReplayRunDataset } from "@/entities/replay/model/types";
import type { RunSummary } from "@/entities/run/model/types";
import { replayPayloadToDataset } from "@/entities/replay/model/adapters";
import type { TelemetrySampleDto } from "@/shared/api/generated/orbital-api";
import { useLiveTelemetryStream } from "@/shared/api/hooks/use-live-telemetry-stream";
import { useReplayQuery } from "@/shared/api/hooks/use-replay-query";
import { useRunsQuery } from "@/shared/api/hooks/use-runs-query";
import { useReplayController } from "@/shared/hooks/use-replay-controller";
import { MISSION_REPLAY_DATASET_BY_ID, MISSION_REPLAY_DATASETS } from "@/shared/mock/mission-replay.mock";
import { MissionTopbar } from "@/widgets/MissionTopbar/ui/mission-topbar";
import { OrbitalCanvasWidget } from "@/widgets/OrbitalCanvas/ui/orbital-canvas-widget";
import { ReplayTimelineWidget } from "@/widgets/ReplayTimeline/ui/replay-timeline-widget";
import { TelemetrySidebarWidget } from "@/widgets/TelemetrySidebar/ui/telemetry-sidebar-widget";

function toRunSummaryFromApi(runId: string): RunSummary {
  const fallback = MISSION_REPLAY_DATASET_BY_ID.get(runId)?.run;
  if (fallback) {
    return fallback;
  }

  return {
    runId,
    label: runId,
    mode: "replay",
    environment: "point_mass",
    backend: "libtorch_cpu",
    deterministic: true,
    totalTimesteps: 0,
    status: "warning",
    artifactStatus: "unknown",
    startedAtIso: new Date().toISOString(),
  };
}

function toLiveFrames(run: RunSummary, samples: TelemetrySampleDto[]): ReplayFrame[] {
  return samples.map((sample, index) => ({
    frameIndex: index,
    timestampIso: sample.timestamp,
    telemetry: {
      runId: run.runId,
      environment: run.environment,
      timestep: sample.step,
      missionTimeS: sample.mission_time_s,
      reward: sample.reward,
      controlMagnitude: sample.control_magnitude,
      orbitalErrorKm: sample.orbital_error_km,
      velocityMagnitudeKmS: sample.velocity_magnitude_kmps,
      policyStd: sample.policy_std,
      backend: run.backend,
      deterministic: run.deterministic,
      positionKm: sample.position_km,
      velocityKmS: sample.velocity_kmps,
      controlVector: sample.control_vector,
      terminated: sample.terminated,
      truncated: sample.truncated,
      timestampIso: sample.timestamp,
    },
    orbit: {
      timestep: sample.step,
      missionTimeS: sample.mission_time_s,
      positionKm: sample.position_km,
      velocityKmS: sample.velocity_kmps,
      orbitalErrorKm: sample.orbital_error_km,
      controlMagnitude: sample.control_magnitude,
      reward: sample.reward,
    },
  }));
}

export function MissionReplayPage(): JSX.Element {
  const defaultDataset = MISSION_REPLAY_DATASETS[0];
  if (!defaultDataset) {
    throw new Error("Mission replay datasets are not configured.");
  }

  const { runs, loading: runsLoading } = useRunsQuery(200);
  const runIds = runs.map((run) => run.run_id);

  const [selectedRunId, setSelectedRunId] = useState(defaultDataset.run.runId);
  const [mode, setMode] = useState<"replay" | "live">("replay");

  useEffect(() => {
    if (runIds.length === 0) {
      return;
    }
    if (!runIds.includes(selectedRunId)) {
      setSelectedRunId(runIds[0] ?? defaultDataset.run.runId);
    }
  }, [defaultDataset.run.runId, runIds, selectedRunId]);

  const { replay: replayPayload } = useReplayQuery(selectedRunId, 1400);
  const liveStream = useLiveTelemetryStream({
    runId: selectedRunId,
    enabled: mode === "live",
    maxSamples: 2200,
  });

  const selectedDataset = useMemo<ReplayRunDataset>(() => {
    if (replayPayload) {
      return replayPayloadToDataset(replayPayload);
    }
    return MISSION_REPLAY_DATASET_BY_ID.get(selectedRunId) ?? defaultDataset;
  }, [defaultDataset, replayPayload, selectedRunId]);

  const liveFrames = useMemo(
    () => toLiveFrames(selectedDataset.run, liveStream.samples),
    [liveStream.samples, selectedDataset.run],
  );

  const dataset = useMemo<ReplayRunDataset>(() => {
    if (mode !== "live" || liveFrames.length === 0) {
      return selectedDataset;
    }

    return {
      ...selectedDataset,
      frames: liveFrames,
      orbitPath: liveFrames.map((frame) => frame.orbit),
      run: {
        ...selectedDataset.run,
        label: `${selectedDataset.run.runId} (live stream)`,
        totalTimesteps: liveFrames.length,
      },
      benchmark: {
        ...selectedDataset.benchmark,
        totalTimesteps: liveFrames.length,
      },
    };
  }, [liveFrames, mode, selectedDataset]);

  const replay = useReplayController({
    totalFrames: dataset.frames.length,
    initialPlaying: true,
  });
  const { reset } = replay;

  useEffect(() => {
    reset();
  }, [mode, reset, selectedRunId]);

  const currentFrame = dataset.frames[replay.frameIndex] ?? dataset.frames[0];

  const runsForTopbar = useMemo<RunSummary[]>(() => {
    if (runs.length === 0) {
      return MISSION_REPLAY_DATASETS.map((item) => item.run);
    }

    return runs.map((run) => ({
      ...toRunSummaryFromApi(run.run_id),
      runId: run.run_id,
      label: run.run_id,
      mode: run.mode,
      environment: run.environment,
      totalTimesteps: selectedRunId === run.run_id ? dataset.frames.length : 0,
      status: run.status === "completed" ? "ok" : run.status === "running" ? "running" : "warning",
      artifactStatus: run.status === "completed" ? "complete" : run.status === "running" ? "partial" : "unknown",
      startedAtIso: run.started_at,
    }));
  }, [dataset.frames.length, runs, selectedRunId]);

  const selectedRun = runsForTopbar.find((run) => run.runId === selectedRunId) ?? dataset.run;

  if (!currentFrame) {
    return (
      <main className="mission-shell">
        <div className="mission-shell__gradient" />
        <div className="mission-shell__content">
          <p className="rounded-lg border border-amber-500/40 bg-amber-500/10 p-4 text-sm text-amber-200">
            Replay data unavailable for the selected run.
          </p>
        </div>
      </main>
    );
  }

  return (
    <main className="mission-shell">
      <div className="mission-shell__gradient" />
      <div className="mission-shell__content">
        <div className="mb-3 flex flex-wrap items-center gap-2 rounded-xl border border-slate-700/65 bg-slate-950/55 px-3 py-2 text-xs uppercase tracking-[0.12em] text-slate-300">
          <span>data source: {runsLoading ? "loading" : runs.length > 0 ? "backend api" : "mock fallback"}</span>
          <span className="text-slate-500">|</span>
          <span>mode:</span>
          <button
            type="button"
            onClick={() => setMode("replay")}
            className={`rounded-full px-3 py-1 text-[11px] font-semibold ${
              mode === "replay" ? "bg-cyan-500/20 text-cyan-100" : "bg-slate-800/80 text-slate-300"
            }`}
          >
            replay
          </button>
          <button
            type="button"
            onClick={() => setMode("live")}
            className={`rounded-full px-3 py-1 text-[11px] font-semibold ${
              mode === "live" ? "bg-cyan-500/20 text-cyan-100" : "bg-slate-800/80 text-slate-300"
            }`}
          >
            live
          </button>
          <span className="text-slate-500">|</span>
          <span>stream: {liveStream.connected ? "connected" : "disconnected"}</span>
          {liveStream.error ? <span className="text-amber-300">{liveStream.error}</span> : null}
        </div>

        <MissionTopbar runs={runsForTopbar} selectedRun={selectedRun} onSelectRun={setSelectedRunId} />

        <section className="mt-5 grid gap-4 xl:grid-cols-[minmax(0,1fr)_340px]">
          <OrbitalCanvasWidget frame={currentFrame} orbitPath={dataset.orbitPath} />
          <TelemetrySidebarWidget frame={currentFrame} run={selectedRun} benchmark={dataset.benchmark} events={dataset.events} />
        </section>

        <section className="mt-4">
          <ReplayTimelineWidget
            frame={currentFrame}
            frameIndex={replay.frameIndex}
            frameCount={replay.frameCount}
            isPlaying={replay.isPlaying}
            speed={replay.speed}
            events={dataset.events}
            onTogglePlay={replay.togglePlaying}
            onReset={reset}
            onFrameChange={replay.setFrameIndex}
            onSpeedChange={replay.setSpeed}
          />
        </section>
      </div>
    </main>
  );
}
