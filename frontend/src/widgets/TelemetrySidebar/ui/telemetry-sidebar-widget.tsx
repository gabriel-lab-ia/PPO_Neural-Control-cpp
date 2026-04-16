import type { ReplayFrame } from "@/entities/replay/model/types";
import type { BenchmarkSummary, MissionEventSummary, RunSummary } from "@/entities/run/model/types";
import { TelemetryInspectionPanel } from "@/features/telemetry-inspection/ui/telemetry-inspection-panel";
import { BenchmarkCardWidget } from "@/widgets/BenchmarkCard/ui/benchmark-card-widget";

interface TelemetrySidebarWidgetProps {
  frame: ReplayFrame;
  run: RunSummary;
  benchmark: BenchmarkSummary;
  events: MissionEventSummary[];
}

export function TelemetrySidebarWidget({ frame, run, benchmark, events }: TelemetrySidebarWidgetProps): JSX.Element {
  return (
    <aside className="flex h-full flex-col gap-4">
      <TelemetryInspectionPanel frame={frame} run={run} events={events} />
      <BenchmarkCardWidget benchmark={benchmark} />
    </aside>
  );
}
