import { Activity, AlertTriangle, ArrowUpRight, Fuel, Gauge } from "lucide-react";

import type { ReplayFrame } from "@/entities/replay/model/types";
import type { MissionEventSummary, RunSummary } from "@/entities/run/model/types";
import { formatBoolean, formatNumber } from "@/shared/lib/format";
import { DataField } from "@/shared/ui/data-field";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

interface TelemetryInspectionPanelProps {
  frame: ReplayFrame;
  run: RunSummary;
  events: MissionEventSummary[];
}

function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

function normalizedPercentage(value: number, max: number): number {
  if (max <= 0) {
    return 0;
  }
  return clamp((value / max) * 100, 0, 100);
}

export function TelemetryInspectionPanel({ frame, run, events }: TelemetryInspectionPanelProps): JSX.Element {
  const { telemetry } = frame;
  const latestEvent = events.length > 0 ? events[events.length - 1] : null;

  const metrics = [
    {
      label: "Reward",
      value: formatNumber(telemetry.reward, 5),
      pct: normalizedPercentage(Math.abs(telemetry.reward), 6),
      icon: <ArrowUpRight className="h-3.5 w-3.5" />,
      detail: "Instant PPO objective signal for this control step.",
    },
    {
      label: "Control Effort",
      value: formatNumber(telemetry.controlMagnitude, 4),
      pct: normalizedPercentage(telemetry.controlMagnitude, 2),
      icon: <Fuel className="h-3.5 w-3.5" />,
      detail: "Quadratic fuel proxy and actuation pressure.",
    },
    {
      label: "Velocity",
      value: `${formatNumber(telemetry.velocityMagnitudeKmS, 4)} km/s`,
      pct: normalizedPercentage(telemetry.velocityMagnitudeKmS, 10),
      icon: <Gauge className="h-3.5 w-3.5" />,
      detail: "Orbital velocity magnitude used for stability diagnostics.",
    },
    {
      label: "Orbital Error",
      value: `${formatNumber(telemetry.orbitalErrorKm, 4)} km`,
      pct: normalizedPercentage(telemetry.orbitalErrorKm, 8),
      icon: <AlertTriangle className="h-3.5 w-3.5" />,
      detail: "Distance deviation from the target orbital corridor.",
    },
  ] as const;

  return (
    <Panel className="h-full">
      <PanelHeader>
        <div className="flex items-center justify-between gap-2">
          <PanelTitle>Telemetry Tracking</PanelTitle>
          <span className="inline-flex items-center gap-1 rounded-full border border-cyan-300/35 bg-cyan-300/12 px-2 py-1 text-[10px] uppercase tracking-[0.16em] text-cyan-100">
            <Activity className="h-3.5 w-3.5" />
            frame {frame.frameIndex + 1}
          </span>
        </div>
      </PanelHeader>

      <PanelBody className="space-y-4">
        <div className="grid gap-2 sm:grid-cols-2">
          {metrics.map((metric) => (
            <article key={metric.label} className="rounded-xl border border-cyan-200/12 bg-black/45 p-3">
              <div className="flex items-center justify-between gap-2 text-xs text-cyan-100">
                <span className="inline-flex items-center gap-1.5 uppercase tracking-[0.14em]">
                  {metric.icon}
                  {metric.label}
                </span>
                <span className="mono text-[11px] text-slate-100">{metric.value}</span>
              </div>
              <div className="metric-bar mt-2">
                <span style={{ width: `${metric.pct.toFixed(1)}%` }} />
              </div>
              <p className="mt-2 text-[11px] text-slate-400">{metric.detail}</p>
            </article>
          ))}
        </div>

        <div className="space-y-0 rounded-xl border border-cyan-200/10 bg-black/50 px-3 py-2">
          <DataField label="run_id" value={telemetry.runId} />
          <DataField label="environment" value={telemetry.environment} />
          <DataField label="timestep" value={telemetry.timestep} />
          <DataField label="control vector" value={telemetry.controlVector.map((value) => formatNumber(value, 3)).join(", ")} />
          <DataField label="backend" value={telemetry.backend} />
          <DataField label="deterministic" value={formatBoolean(telemetry.deterministic)} />
          <DataField label="artifact status" value={run.artifactStatus} />
          <DataField label="policy std" value={formatNumber(telemetry.policyStd, 5)} />
          <DataField label="latest event" value={latestEvent ? `${latestEvent.eventType} (${latestEvent.level})` : "none"} />
        </div>

        <div className="rounded-xl border border-cyan-200/12 bg-black/45 p-3">
          <div className="mb-2 text-[11px] uppercase tracking-[0.18em] text-slate-400">Technical Table</div>
          <div className="overflow-x-auto">
            <table className="w-full min-w-[300px] border-collapse text-xs text-slate-300">
              <thead>
                <tr className="border-b border-cyan-200/15 text-[10px] uppercase tracking-[0.14em] text-slate-400">
                  <th className="py-1.5 pr-2 text-left">metric</th>
                  <th className="py-1.5 pr-2 text-left">value</th>
                  <th className="py-1.5 text-left">engineering interpretation</th>
                </tr>
              </thead>
              <tbody>
                {metrics.map((metric) => (
                  <tr key={metric.label} className="border-b border-cyan-200/10 last:border-b-0">
                    <td className="py-1.5 pr-2 font-medium text-cyan-100">{metric.label.toLowerCase().replaceAll(" ", "_")}</td>
                    <td className="mono py-1.5 pr-2 text-slate-100">{metric.value}</td>
                    <td className="py-1.5 text-slate-300">{metric.detail}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </PanelBody>
    </Panel>
  );
}
