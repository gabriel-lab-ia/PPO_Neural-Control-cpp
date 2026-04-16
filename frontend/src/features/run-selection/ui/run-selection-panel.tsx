import type { RunSummary } from "@/entities/run/model/types";
import { formatTimestamp } from "@/shared/lib/format";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

interface RunSelectionPanelProps {
  runs: RunSummary[];
  selectedRunId: string;
  onSelectRun: (runId: string) => void;
}

export function RunSelectionPanel({ runs, selectedRunId, onSelectRun }: RunSelectionPanelProps): JSX.Element {
  const selected = runs.find((run) => run.runId === selectedRunId) ?? runs[0];

  return (
    <Panel>
      <PanelHeader>
        <PanelTitle>Run Selection</PanelTitle>
      </PanelHeader>
      <PanelBody className="space-y-3">
        <label className="block text-[11px] uppercase tracking-[0.16em] text-slate-400" htmlFor="run-selector">
          Mission Replay Source
        </label>
        <select
          id="run-selector"
          value={selectedRunId}
          onChange={(event) => onSelectRun(event.target.value)}
          className="w-full rounded-lg border border-slate-700 bg-slate-950/80 px-3 py-2 text-sm text-slate-100 outline-none ring-cyan-300 transition focus:ring-2"
        >
          {runs.map((run) => (
            <option key={run.runId} value={run.runId}>
              {run.label}
            </option>
          ))}
        </select>

        {selected ? (
          <dl className="space-y-1 text-xs text-slate-300">
            <div className="flex justify-between gap-3">
              <dt className="text-slate-400">run_id</dt>
              <dd>{selected.runId}</dd>
            </div>
            <div className="flex justify-between gap-3">
              <dt className="text-slate-400">environment</dt>
              <dd>{selected.environment}</dd>
            </div>
            <div className="flex justify-between gap-3">
              <dt className="text-slate-400">started</dt>
              <dd>{formatTimestamp(selected.startedAtIso)}</dd>
            </div>
          </dl>
        ) : null}
      </PanelBody>
    </Panel>
  );
}
