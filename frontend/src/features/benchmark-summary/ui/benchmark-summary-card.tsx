import type { BenchmarkSummary } from "@/entities/run/model/types";
import { formatBoolean, formatNumber } from "@/shared/lib/format";
import { DataField } from "@/shared/ui/data-field";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

interface BenchmarkSummaryCardProps {
  benchmark: BenchmarkSummary;
}

export function BenchmarkSummaryCard({ benchmark }: BenchmarkSummaryCardProps): JSX.Element {
  return (
    <Panel>
      <PanelHeader>
        <PanelTitle>Benchmark Summary</PanelTitle>
      </PanelHeader>
      <PanelBody className="space-y-0">
        <DataField label="total timesteps" value={benchmark.totalTimesteps} />
        <DataField label="mean reward" value={formatNumber(benchmark.meanReward, 4)} />
        <DataField label="deterministic" value={formatBoolean(benchmark.deterministic)} />
        <DataField label="backend" value={benchmark.backend} />
        <DataField label="artifact status" value={benchmark.artifactStatus} />
      </PanelBody>
    </Panel>
  );
}
