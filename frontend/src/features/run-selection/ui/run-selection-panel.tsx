import { Clock3, Database, Rocket } from "lucide-react";

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
        <PanelTitle>Run Selector</PanelTitle>
      </PanelHeader>
      <PanelBody className="space-y-4">
        <label className="hud-label block" htmlFor="run-selector">
          Mission Replay Source
        </label>

        <select
          id="run-selector"
          value={selectedRunId}
          onChange={(event) => onSelectRun(event.target.value)}
          className="w-full rounded-xl border border-cyan-200/25 bg-black/55 px-3 py-2 text-sm text-slate-100 outline-none ring-cyan-200 transition focus:ring-2"
        >
          {runs.map((run) => (
            <option key={run.runId} value={run.runId}>
              {run.label}
            </option>
          ))}
        </select>

        {selected ? (
          <dl className="space-y-2 rounded-xl border border-cyan-200/15 bg-black/45 p-3 text-xs text-slate-300">
            <MetaRow icon={<Database className="h-3.5 w-3.5" />} label="run_id" value={selected.runId} />
            <MetaRow icon={<Rocket className="h-3.5 w-3.5" />} label="environment" value={selected.environment} />
            <MetaRow icon={<Clock3 className="h-3.5 w-3.5" />} label="started" value={formatTimestamp(selected.startedAtIso)} />
          </dl>
        ) : null}
      </PanelBody>
    </Panel>
  );
}

interface MetaRowProps {
  icon: JSX.Element;
  label: string;
  value: string;
}

function MetaRow({ icon, label, value }: MetaRowProps): JSX.Element {
  return (
    <div className="flex items-center justify-between gap-3">
      <dt className="flex items-center gap-1.5 text-slate-400">
        {icon}
        <span className="hud-label">{label}</span>
      </dt>
      <dd className="mono text-[11px] text-slate-100">{value}</dd>
    </div>
  );
}
