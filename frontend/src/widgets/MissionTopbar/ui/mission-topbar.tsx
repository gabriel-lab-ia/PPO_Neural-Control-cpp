import { Activity, Cpu, Orbit, Radar, ShieldCheck } from "lucide-react";
import { Link } from "react-router-dom";

import type { RunSummary } from "@/entities/run/model/types";
import { RunSelectionPanel } from "@/features/run-selection/ui/run-selection-panel";
import { StatusChip } from "@/shared/ui/status-chip";

interface MissionTopbarProps {
  runs: RunSummary[];
  selectedRun: RunSummary;
  onSelectRun: (runId: string) => void;
}

const PRIMARY_TABS = [
  { id: "orbital-sim", label: "Orbital Simulation", icon: Orbit },
  { id: "ppo-control", label: "PPO Control", icon: Cpu },
  { id: "telemetry", label: "Telemetry Tracking", icon: Radar },
  { id: "deterministic", label: "Deterministic Baseline", icon: ShieldCheck },
] as const;

export function MissionTopbar({ runs, selectedRun, onSelectRun }: MissionTopbarProps): JSX.Element {
  return (
    <header className="grid gap-4 2xl:grid-cols-[minmax(0,1fr)_360px]">
      <section className="space-y-4">
        <div className="hud-panel glow-border rounded-2xl border border-cyan-200/15 px-5 py-4">
          <div className="flex flex-wrap items-start justify-between gap-3">
            <div className="space-y-2">
              <p className="text-xs uppercase tracking-[0.28em] text-cyan-200/85">Neural Orbit Control</p>
              <h1 className="text-2xl font-semibold uppercase tracking-[0.12em] text-white sm:text-[2.15rem]">
                OrbitalForge Mission Control
              </h1>
              <p className="max-w-3xl text-sm text-slate-300 sm:text-[0.95rem]">
                High-fidelity replay and live telemetry viewport for deterministic PPO orbital control runs.
              </p>
            </div>

            <div className="rounded-xl border border-cyan-300/25 bg-cyan-300/10 px-3 py-2 text-xs text-cyan-100">
              <div className="mb-1 flex items-center gap-2">
                <Activity className="h-3.5 w-3.5" />
                <span className="uppercase tracking-[0.15em]">Mission Feed</span>
              </div>
              <div className="mono text-[11px] text-cyan-50/95">run_id: {selectedRun.runId}</div>
            </div>
          </div>

          <div className="mt-4 flex flex-wrap items-center gap-2">
            {PRIMARY_TABS.map((tab) => {
              const Icon = tab.icon;
              return (
                <span key={tab.id} className="neon-pill inline-flex items-center gap-1.5 rounded-full px-3 py-1.5 text-[10px] font-semibold uppercase tracking-[0.16em]">
                  <Icon className="h-3.5 w-3.5" />
                  {tab.label}
                </span>
              );
            })}

            <Link
              to="/runs"
              className="inline-flex items-center rounded-full border border-violet-300/40 bg-violet-500/10 px-3 py-1.5 text-[10px] font-semibold uppercase tracking-[0.16em] text-violet-100 transition hover:border-violet-200/70 hover:bg-violet-500/18"
            >
              Run Explorer
            </Link>
          </div>

          <div className="mt-4 grid gap-2 sm:grid-cols-2 lg:grid-cols-4">
            <HudBadge label="run_id" value={selectedRun.runId} />
            <HudBadge label="environment" value={selectedRun.environment} />
            <HudBadge label="backend" value={selectedRun.backend} />
            <HudBadge label="mode" value={selectedRun.mode} />
          </div>

          <div className="mt-3 flex flex-wrap gap-2">
            <StatusChip tone={selectedRun.deterministic ? "ok" : "warning"}>
              deterministic {selectedRun.deterministic ? "enabled" : "disabled"}
            </StatusChip>
            <StatusChip tone={selectedRun.status === "ok" ? "ok" : selectedRun.status === "running" ? "default" : "warning"}>
              runtime {selectedRun.status}
            </StatusChip>
            <StatusChip tone={selectedRun.artifactStatus === "complete" ? "ok" : "warning"}>
              artifact {selectedRun.artifactStatus}
            </StatusChip>
          </div>
        </div>
      </section>

      <RunSelectionPanel runs={runs} selectedRunId={selectedRun.runId} onSelectRun={onSelectRun} />
    </header>
  );
}

interface HudBadgeProps {
  label: string;
  value: string;
}

function HudBadge({ label, value }: HudBadgeProps): JSX.Element {
  return (
    <div className="rounded-xl border border-cyan-200/15 bg-black/45 px-3 py-2">
      <p className="hud-label">{label}</p>
      <p className="mono mt-1 text-sm text-slate-100">{value}</p>
    </div>
  );
}
