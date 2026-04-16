import type { ReplayFrame } from "@/entities/replay/model/types";
import type { RunSummary } from "@/entities/run/model/types";
import { formatBoolean, formatNumber } from "@/shared/lib/format";
import { DataField } from "@/shared/ui/data-field";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

interface TelemetryInspectionPanelProps {
  frame: ReplayFrame;
  run: RunSummary;
}

export function TelemetryInspectionPanel({ frame, run }: TelemetryInspectionPanelProps): JSX.Element {
  const { telemetry } = frame;

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
        <DataField label="backend" value={telemetry.backend} />
        <DataField label="deterministic" value={formatBoolean(telemetry.deterministic)} />
        <DataField label="artifact status" value={run.artifactStatus} />
      </PanelBody>
    </Panel>
  );
}
