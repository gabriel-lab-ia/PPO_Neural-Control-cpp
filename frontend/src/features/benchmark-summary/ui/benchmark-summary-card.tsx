import type { BenchmarkSummary } from "@/entities/run/model/types";
import { formatBoolean, formatNumber } from "@/shared/lib/format";
import { DataField } from "@/shared/ui/data-field";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

interface BenchmarkSummaryCardProps {
  benchmark: BenchmarkSummary;
}

export function BenchmarkSummaryCard({ benchmark }: BenchmarkSummaryCardProps): JSX.Element {
  const benchmarkRows = [
    {
      metric: "mean_reward",
      value: formatNumber(benchmark.meanReward, 5),
      detail: "Average reward across replay window.",
    },
    {
      metric: "timesteps",
      value: String(benchmark.totalTimesteps),
      detail: "Number of propagated samples in this mission trace.",
    },
    {
      metric: "deterministic",
      value: formatBoolean(benchmark.deterministic),
      detail: "Same seed and config should reproduce equivalent trace.",
    },
  ];

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
        <div className="mt-4 border-t border-slate-800/80 pt-3">
          <div className="mb-2 text-[11px] uppercase tracking-[0.14em] text-slate-400">Benchmark Technical Table</div>
          <table className="w-full border-collapse text-xs text-slate-300">
            <thead>
              <tr className="border-b border-slate-700/70 text-[10px] uppercase tracking-[0.12em] text-slate-400">
                <th className="py-1 pr-2 text-left">metric</th>
                <th className="py-1 pr-2 text-left">value</th>
                <th className="py-1 text-left">detail</th>
              </tr>
            </thead>
            <tbody>
              {benchmarkRows.map((row) => (
                <tr key={row.metric} className="border-b border-slate-800/70">
                  <td className="py-1 pr-2 font-medium text-cyan-100">{row.metric}</td>
                  <td className="py-1 pr-2">{row.value}</td>
                  <td className="py-1">{row.detail}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </PanelBody>
    </Panel>
  );
}
