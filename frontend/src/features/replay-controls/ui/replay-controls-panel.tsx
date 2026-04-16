"use client";

import { Pause, Play, RotateCcw } from "lucide-react";

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
            className="inline-flex items-center gap-2 rounded-lg border border-cyan-400/40 bg-cyan-400/12 px-3 py-1.5 text-sm font-medium text-cyan-100 transition hover:bg-cyan-400/18"
          >
            {isPlaying ? <Pause className="h-4 w-4" /> : <Play className="h-4 w-4" />}
            {isPlaying ? "Pause" : "Play"}
          </button>

          <button
            type="button"
            onClick={onReset}
            className="inline-flex items-center gap-2 rounded-lg border border-slate-600 bg-slate-800/70 px-3 py-1.5 text-sm font-medium text-slate-100 transition hover:bg-slate-700/75"
          >
            <RotateCcw className="h-4 w-4" />
            Reset
          </button>

          <label className="ml-auto flex items-center gap-2 text-sm text-slate-300">
            speed
            <select
              value={String(speed)}
              onChange={(event) => onSpeedChange(Number(event.target.value))}
              className="rounded-md border border-slate-700 bg-slate-950/80 px-2 py-1 text-sm text-slate-100 outline-none ring-cyan-300 focus:ring-2"
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
            <span>timeline</span>
            <span>
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
            className="h-2 w-full cursor-pointer appearance-none rounded-lg bg-slate-800"
          />
        </div>
      </PanelBody>
    </Panel>
  );
}
