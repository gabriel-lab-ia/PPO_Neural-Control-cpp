import { Link } from "react-router-dom";

import type { RunSummary } from "@/entities/run/model/types";
import { RunSelectionPanel } from "@/features/run-selection/ui/run-selection-panel";
import { StatusChip } from "@/shared/ui/status-chip";

interface MissionTopbarProps {
  runs: RunSummary[];
  selectedRun: RunSummary;
  onSelectRun: (runId: string) => void;
}

export function MissionTopbar({ runs, selectedRun, onSelectRun }: MissionTopbarProps): JSX.Element {
  return (
    <header className="grid gap-4 xl:grid-cols-[1fr_320px]">
      <section className="space-y-4">
        <div className="space-y-2">
          <p className="text-xs uppercase tracking-[0.2em] text-cyan-300/85">Orbital Autonomy Mission Replay</p>
          <h1 className="text-3xl font-semibold tracking-tight text-slate-50 md:text-4xl">Mission Control Replay Console</h1>
          <p className="max-w-3xl text-sm text-slate-300 md:text-base">
            Replay deterministic PPO control runs with orbital trajectory context, technical telemetry inspection, and
            benchmark traceability.
          </p>
        </div>

        <div className="flex flex-wrap items-center gap-2">
          <StatusChip>Orbital Simulation</StatusChip>
          <StatusChip>PPO Control</StatusChip>
          <StatusChip>Telemetry Tracking</StatusChip>
          <StatusChip tone="ok">Deterministic Baseline</StatusChip>
          <Link
            to="/runs"
            className="inline-flex items-center rounded-full border border-slate-600 px-3 py-1 text-[11px] font-semibold uppercase tracking-[0.14em] text-slate-200 transition hover:border-cyan-400/60 hover:text-cyan-200"
          >
            Run Explorer
          </Link>
        </div>

        <div className="grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
          <div className="rounded-xl border border-slate-700/70 bg-slate-950/45 px-3 py-2">
            <p className="text-[11px] uppercase tracking-[0.14em] text-slate-400">run_id</p>
            <p className="mt-1 text-sm font-medium text-slate-100">{selectedRun.runId}</p>
          </div>
          <div className="rounded-xl border border-slate-700/70 bg-slate-950/45 px-3 py-2">
            <p className="text-[11px] uppercase tracking-[0.14em] text-slate-400">mode</p>
            <p className="mt-1 text-sm font-medium text-slate-100">{selectedRun.mode}</p>
          </div>
          <div className="rounded-xl border border-slate-700/70 bg-slate-950/45 px-3 py-2">
            <p className="text-[11px] uppercase tracking-[0.14em] text-slate-400">environment</p>
            <p className="mt-1 text-sm font-medium text-slate-100">{selectedRun.environment}</p>
          </div>
          <div className="rounded-xl border border-slate-700/70 bg-slate-950/45 px-3 py-2">
            <p className="text-[11px] uppercase tracking-[0.14em] text-slate-400">backend</p>
            <p className="mt-1 text-sm font-medium text-slate-100">{selectedRun.backend}</p>
          </div>
        </div>
      </section>

      <RunSelectionPanel runs={runs} selectedRunId={selectedRun.runId} onSelectRun={onSelectRun} />
    </header>
  );
}
