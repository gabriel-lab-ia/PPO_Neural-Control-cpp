"use client";

import { Gauge, Pause, Play, RotateCcw } from "lucide-react";

import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

interface ReplayControlsPanelProps {
  frameIndex: number;
  frameCount: number;
  isPlaying: boolean;
  speed: number;
  speedOptions: readonly number[];
  onTogglePlay: () => void;
  onReset: () => void;
  onFrameChange: (frameIndex: number) => void;
  onSpeedChange: (speed: number) => void;
}

export function ReplayControlsPanel({
  frameIndex,
  frameCount,
  isPlaying,
  speed,
  speedOptions,
  onTogglePlay,
  onReset,
  onFrameChange,
  onSpeedChange,
}: ReplayControlsPanelProps): JSX.Element {
  return (
    <Panel>
      <PanelHeader>
        <PanelTitle>Replay Controls</PanelTitle>
      </PanelHeader>
      <PanelBody className="space-y-4">
        <div className="flex flex-wrap items-center gap-3">
          <button
            type="button"
            onClick={onTogglePlay}
            className="inline-flex items-center gap-2 rounded-lg border border-cyan-300/45 bg-cyan-300/12 px-3 py-1.5 text-sm font-medium text-cyan-100 transition hover:bg-cyan-300/20"
          >
            {isPlaying ? <Pause className="h-4 w-4" /> : <Play className="h-4 w-4" />}
            {isPlaying ? "Pause" : "Play"}
          </button>

          <button
            type="button"
            onClick={onReset}
            className="inline-flex items-center gap-2 rounded-lg border border-slate-500/70 bg-black/55 px-3 py-1.5 text-sm font-medium text-slate-100 transition hover:border-cyan-300/50"
          >
            <RotateCcw className="h-4 w-4" />
            Reset
          </button>

          <label className="ml-auto flex items-center gap-2 text-xs uppercase tracking-[0.16em] text-slate-300">
            <Gauge className="h-3.5 w-3.5" />
            speed
            <select
              value={String(speed)}
              onChange={(event) => onSpeedChange(Number(event.target.value))}
              className="rounded-md border border-cyan-200/30 bg-black/70 px-2 py-1 text-sm text-slate-100 outline-none ring-cyan-200 focus:ring-2"
            >
              {speedOptions.map((option) => (
                <option key={option} value={option}>
                  {option}x
                </option>
              ))}
            </select>
          </label>
        </div>

        <div className="space-y-2">
          <div className="flex items-center justify-between text-xs uppercase tracking-[0.14em] text-slate-400">
            <span>timeline scrubber</span>
            <span className="mono text-slate-200">
              frame {frameIndex + 1} / {frameCount}
            </span>
          </div>
          <input
            type="range"
            min={0}
            max={Math.max(0, frameCount - 1)}
            step={1}
            value={frameIndex}
            onChange={(event) => onFrameChange(Number(event.target.value))}
            className="h-2 w-full cursor-pointer appearance-none"
          />
        </div>
      </PanelBody>
    </Panel>
  );
}
