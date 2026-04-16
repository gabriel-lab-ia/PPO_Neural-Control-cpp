import type { BenchmarkSummary } from "@/entities/run/model/types";
import { BenchmarkSummaryCard } from "@/features/benchmark-summary/ui/benchmark-summary-card";

interface BenchmarkCardWidgetProps {
  benchmark: BenchmarkSummary;
}

export function BenchmarkCardWidget({ benchmark }: BenchmarkCardWidgetProps): JSX.Element {
  return <BenchmarkSummaryCard benchmark={benchmark} />;
}
