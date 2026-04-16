import { BarChart3, CheckCircle2 } from "lucide-react";

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
        <div className="flex items-center justify-between gap-2">
          <PanelTitle>Benchmark Summary</PanelTitle>
          <span className="inline-flex items-center gap-1 rounded-full border border-cyan-300/35 bg-cyan-300/12 px-2 py-1 text-[10px] uppercase tracking-[0.16em] text-cyan-100">
            <BarChart3 className="h-3.5 w-3.5" />
            regression view
          </span>
        </div>
      </PanelHeader>

      <PanelBody className="space-y-4">
        <div className="space-y-0 rounded-xl border border-cyan-200/10 bg-black/45 px-3 py-2">
          <DataField label="total timesteps" value={benchmark.totalTimesteps} />
          <DataField label="mean reward" value={formatNumber(benchmark.meanReward, 4)} />
          <DataField label="deterministic" value={formatBoolean(benchmark.deterministic)} />
          <DataField label="backend" value={benchmark.backend} />
          <DataField label="artifact status" value={benchmark.artifactStatus} />
        </div>

        <div className="rounded-xl border border-cyan-200/12 bg-black/45 p-3">
          <div className="mb-2 text-[11px] uppercase tracking-[0.14em] text-slate-400">Benchmark Technical Table</div>
          <table className="w-full border-collapse text-xs text-slate-300">
            <thead>
              <tr className="border-b border-cyan-200/15 text-[10px] uppercase tracking-[0.12em] text-slate-400">
                <th className="py-1 pr-2 text-left">metric</th>
                <th className="py-1 pr-2 text-left">value</th>
                <th className="py-1 text-left">detail</th>
              </tr>
            </thead>
            <tbody>
              {benchmarkRows.map((row) => (
                <tr key={row.metric} className="border-b border-cyan-200/10 last:border-b-0">
                  <td className="py-1.5 pr-2 font-medium text-cyan-100">{row.metric}</td>
                  <td className="mono py-1.5 pr-2 text-slate-100">{row.value}</td>
                  <td className="py-1.5">{row.detail}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>

        <div className="inline-flex items-center gap-1.5 rounded-full border border-emerald-300/35 bg-emerald-400/10 px-3 py-1 text-[10px] uppercase tracking-[0.16em] text-emerald-100">
          <CheckCircle2 className="h-3.5 w-3.5" />
          benchmark contract intact
        </div>
      </PanelBody>
    </Panel>
  );
}
