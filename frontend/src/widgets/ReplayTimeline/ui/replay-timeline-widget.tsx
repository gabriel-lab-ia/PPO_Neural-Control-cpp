import type { ReplayFrame } from "@/entities/replay/model/types";
import { ReplayControlsPanel } from "@/features/replay-controls/ui/replay-controls-panel";
import { REPLAY_SPEED_OPTIONS } from "@/shared/config/replay";
import { formatTimestamp } from "@/shared/lib/format";

interface ReplayTimelineWidgetProps {
  frame: ReplayFrame;
  frameIndex: number;
  frameCount: number;
  isPlaying: boolean;
  speed: number;
  onTogglePlay: () => void;
  onReset: () => void;
  onFrameChange: (frameIndex: number) => void;
  onSpeedChange: (speed: number) => void;
}

export function ReplayTimelineWidget({
  frame,
  frameIndex,
  frameCount,
  isPlaying,
  speed,
  onTogglePlay,
  onReset,
  onFrameChange,
  onSpeedChange,
}: ReplayTimelineWidgetProps): JSX.Element {
  return (
    <div className="space-y-3">
      <div className="flex flex-wrap items-center justify-between gap-2 rounded-xl border border-slate-700/65 bg-slate-900/55 px-4 py-2 text-xs uppercase tracking-[0.14em] text-slate-300">
        <span>mission time: {Math.round(frame.telemetry.missionTimeS)} s</span>
        <span>timestamp: {formatTimestamp(frame.timestampIso)}</span>
        <span>
          frame: {frameIndex + 1}/{frameCount}
        </span>
      </div>

      <ReplayControlsPanel
        frameIndex={frameIndex}
        frameCount={frameCount}
        isPlaying={isPlaying}
        speed={speed}
        speedOptions={REPLAY_SPEED_OPTIONS}
        onTogglePlay={onTogglePlay}
        onReset={onReset}
        onFrameChange={onFrameChange}
        onSpeedChange={onSpeedChange}
      />
    </div>
  );
}
