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

export function TelemetryInspectionPanel({ frame, run, events }: TelemetryInspectionPanelProps): JSX.Element {
  const { telemetry } = frame;
  const latestEvent = events.length > 0 ? events[events.length - 1] : null;
  const technicalRows = [
    {
      metric: "reward",
      value: formatNumber(telemetry.reward, 5),
      detail: "Instant PPO objective signal at this timestep.",
    },
    {
      metric: "orbital_error_km",
      value: formatNumber(telemetry.orbitalErrorKm, 4),
      detail: "Distance deviation from nominal reference orbit.",
    },
    {
      metric: "control_magnitude",
      value: formatNumber(telemetry.controlMagnitude, 4),
      detail: "Proxy for thruster effort / fuel usage pressure.",
    },
    {
      metric: "velocity_km_s",
      value: formatNumber(telemetry.velocityMagnitudeKmS, 4),
      detail: "Orbital velocity magnitude used in stability diagnostics.",
    },
    {
      metric: "policy_std",
      value: formatNumber(telemetry.policyStd, 5),
      detail: "Action-distribution spread (exploration/exploitation mix).",
    },
  ];

  return (
    <Panel className="h-full">
      <PanelHeader>
        <PanelTitle>Telemetry Inspection</PanelTitle>
      </PanelHeader>
      <PanelBody className="space-y-0">
        <DataField label="run_id" value={telemetry.runId} />
        <DataField label="environment" value={telemetry.environment} />
        <DataField label="timestep" value={telemetry.timestep} />
        <DataField label="reward" value={formatNumber(telemetry.reward, 4)} />
        <DataField label="control effort" value={formatNumber(telemetry.controlMagnitude, 4)} />
        <DataField label="velocity magnitude" value={`${formatNumber(telemetry.velocityMagnitudeKmS, 4)} km/s`} />
        <DataField label="orbital error" value={`${formatNumber(telemetry.orbitalErrorKm, 3)} km`} />
        <DataField label="control vector" value={telemetry.controlVector.map((value) => formatNumber(value, 3)).join(", ")} />
        <DataField label="backend" value={telemetry.backend} />
        <DataField label="deterministic" value={formatBoolean(telemetry.deterministic)} />
        <DataField label="artifact status" value={run.artifactStatus} />
        <DataField label="latest event" value={latestEvent ? `${latestEvent.eventType} (${latestEvent.level})` : "none"} />

        <div className="mt-4 border-t border-slate-800/80 pt-3">
          <div className="mb-2 text-[11px] uppercase tracking-[0.14em] text-slate-400">Technical Detail Table</div>
          <div className="overflow-x-auto">
            <table className="w-full min-w-[300px] border-collapse text-xs text-slate-300">
              <thead>
                <tr className="border-b border-slate-700/70 text-[10px] uppercase tracking-[0.12em] text-slate-400">
                  <th className="py-1 pr-2 text-left">metric</th>
                  <th className="py-1 pr-2 text-left">value</th>
                  <th className="py-1 text-left">engineering interpretation</th>
                </tr>
              </thead>
              <tbody>
                {technicalRows.map((row) => (
                  <tr key={row.metric} className="border-b border-slate-800/70">
                    <td className="py-1 pr-2 font-medium text-cyan-100">{row.metric}</td>
                    <td className="py-1 pr-2">{row.value}</td>
                    <td className="py-1">{row.detail}</td>
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
