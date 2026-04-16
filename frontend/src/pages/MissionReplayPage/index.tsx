"use client";

import { useEffect, useMemo, useState } from "react";

import { MISSION_REPLAY_DATASETS, MISSION_REPLAY_DATASET_BY_ID } from "@/shared/mock/mission-replay.mock";
import { useReplayController } from "@/shared/hooks/use-replay-controller";
import { MissionTopbar } from "@/widgets/MissionTopbar/ui/mission-topbar";
import { OrbitalCanvasWidget } from "@/widgets/OrbitalCanvas/ui/orbital-canvas-widget";
import { ReplayTimelineWidget } from "@/widgets/ReplayTimeline/ui/replay-timeline-widget";
import { TelemetrySidebarWidget } from "@/widgets/TelemetrySidebar/ui/telemetry-sidebar-widget";

export function MissionReplayPage(): JSX.Element {
  const defaultDataset = MISSION_REPLAY_DATASETS[0];
  if (!defaultDataset) {
    throw new Error("Mission replay datasets are not configured.");
  }

  const [selectedRunId, setSelectedRunId] = useState(defaultDataset.run.runId);

  const selectedDataset = useMemo(
    () => MISSION_REPLAY_DATASET_BY_ID.get(selectedRunId) ?? defaultDataset,
    [defaultDataset, selectedRunId],
  );

  const replay = useReplayController({
    totalFrames: selectedDataset.frames.length,
    initialPlaying: true,
  });
  const { reset } = replay;

  useEffect(() => {
    reset();
  }, [reset, selectedRunId]);

  const currentFrame = selectedDataset.frames[replay.frameIndex] ?? selectedDataset.frames[0];

  return (
    <main className="mission-shell">
      <div className="mission-shell__gradient" />
      <div className="mission-shell__content">
        <MissionTopbar
          runs={MISSION_REPLAY_DATASETS.map((dataset) => dataset.run)}
          selectedRun={selectedDataset.run}
          onSelectRun={setSelectedRunId}
        />

        <section className="mt-5 grid gap-4 xl:grid-cols-[minmax(0,1fr)_340px]">
          <OrbitalCanvasWidget frame={currentFrame} orbitPath={selectedDataset.orbitPath} />
          <TelemetrySidebarWidget frame={currentFrame} run={selectedDataset.run} benchmark={selectedDataset.benchmark} />
        </section>

        <section className="mt-4">
          <ReplayTimelineWidget
            frame={currentFrame}
            frameIndex={replay.frameIndex}
            frameCount={replay.frameCount}
            isPlaying={replay.isPlaying}
            speed={replay.speed}
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
