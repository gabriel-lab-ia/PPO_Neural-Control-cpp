import type { ReplayFrame } from "@/entities/replay/model/types";
import type { BenchmarkSummary, RunSummary } from "@/entities/run/model/types";
import { TelemetryInspectionPanel } from "@/features/telemetry-inspection/ui/telemetry-inspection-panel";
import { BenchmarkCardWidget } from "@/widgets/BenchmarkCard/ui/benchmark-card-widget";

interface TelemetrySidebarWidgetProps {
  frame: ReplayFrame;
  run: RunSummary;
  benchmark: BenchmarkSummary;
}

export function TelemetrySidebarWidget({ frame, run, benchmark }: TelemetrySidebarWidgetProps): JSX.Element {
  return (
    <aside className="flex h-full flex-col gap-4">
      <TelemetryInspectionPanel frame={frame} run={run} />
      <BenchmarkCardWidget benchmark={benchmark} />
    </aside>
  );
}
