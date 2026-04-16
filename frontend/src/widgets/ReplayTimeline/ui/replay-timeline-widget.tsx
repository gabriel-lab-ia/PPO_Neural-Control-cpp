import { CalendarClock, FlagTriangleRight } from "lucide-react";

import type { ReplayFrame } from "@/entities/replay/model/types";
import type { MissionEventSummary } from "@/entities/run/model/types";
import { ReplayControlsPanel } from "@/features/replay-controls/ui/replay-controls-panel";
import { REPLAY_SPEED_OPTIONS } from "@/shared/config/replay";
import { formatTimestamp } from "@/shared/lib/format";

interface ReplayTimelineWidgetProps {
  frame: ReplayFrame;
  frameIndex: number;
  frameCount: number;
  isPlaying: boolean;
  speed: number;
  events: MissionEventSummary[];
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
  events,
  onTogglePlay,
  onReset,
  onFrameChange,
  onSpeedChange,
}: ReplayTimelineWidgetProps): JSX.Element {
  const eventMarkers = events.map((event, index) => {
    const position = frameCount <= 1 ? 0 : Math.round((index / Math.max(1, events.length - 1)) * (frameCount - 1));
    return { event, position };
  });

  return (
    <div className="space-y-3">
      <div className="hud-panel glow-border flex flex-wrap items-center justify-between gap-2 rounded-xl border border-cyan-200/15 px-4 py-2 text-xs uppercase tracking-[0.14em] text-slate-300">
        <span className="inline-flex items-center gap-1.5">
          <FlagTriangleRight className="h-3.5 w-3.5 text-cyan-200" />
          mission time: {Math.round(frame.telemetry.missionTimeS)} s
        </span>
        <span className="inline-flex items-center gap-1.5">
          <CalendarClock className="h-3.5 w-3.5 text-cyan-200" />
          timestamp: {formatTimestamp(frame.timestampIso)}
        </span>
        <span className="mono text-slate-100">
          frame: {frameIndex + 1}/{frameCount}
        </span>
      </div>

      {eventMarkers.length > 0 ? (
        <div className="hud-panel glow-border rounded-xl border border-cyan-200/15 px-4 py-3">
          <div className="mb-2 text-[11px] uppercase tracking-[0.18em] text-slate-400">Event Markers</div>
          <div className="relative h-8 rounded-md border border-cyan-200/12 bg-black/70">
            {eventMarkers.map(({ event, position }) => (
              <button
                key={event.id}
                type="button"
                onClick={() => onFrameChange(position)}
                className="absolute top-1/2 h-4 w-1 -translate-y-1/2 rounded bg-cyan-300/90 shadow-[0_0_8px_rgba(103,232,249,0.75)] hover:bg-cyan-100"
                style={{ left: `${(position / Math.max(1, frameCount - 1)) * 100}%` }}
                title={`${event.eventType}: ${event.message}`}
              />
            ))}
          </div>
        </div>
      ) : null}

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
