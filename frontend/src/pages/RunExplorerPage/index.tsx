import { Link } from "react-router-dom";

import { MISSION_REPLAY_DATASETS } from "@/shared/mock/mission-replay.mock";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

export function RunExplorerPage(): JSX.Element {
  return (
    <main className="mission-shell">
      <div className="mission-shell__gradient" />
      <div className="mission-shell__content space-y-4">
        <header className="flex items-center justify-between gap-3">
          <div>
            <p className="text-xs uppercase tracking-[0.2em] text-cyan-300/85">Run Explorer</p>
            <h1 className="text-3xl font-semibold text-slate-50">Replay Dataset Registry</h1>
          </div>
          <Link
            to="/"
            className="rounded-lg border border-slate-700 bg-slate-900/65 px-3 py-2 text-sm font-medium text-slate-100 transition hover:border-cyan-400/60 hover:text-cyan-200"
          >
            Back to Mission Replay
          </Link>
        </header>

        <Panel>
          <PanelHeader>
            <PanelTitle>Available Runs</PanelTitle>
          </PanelHeader>
          <PanelBody className="overflow-x-auto">
            <table className="w-full min-w-[760px] border-collapse text-left text-sm text-slate-200">
              <thead>
                <tr className="border-b border-slate-700/70 text-[11px] uppercase tracking-[0.14em] text-slate-400">
                  <th className="py-2 pr-3">run_id</th>
                  <th className="py-2 pr-3">label</th>
                  <th className="py-2 pr-3">environment</th>
                  <th className="py-2 pr-3">timesteps</th>
                  <th className="py-2 pr-3">backend</th>
                  <th className="py-2 pr-3">deterministic</th>
                  <th className="py-2 pr-3">artifact status</th>
                </tr>
              </thead>
              <tbody>
                {MISSION_REPLAY_DATASETS.map((dataset) => (
                  <tr key={dataset.run.runId} className="border-b border-slate-800/70">
                    <td className="py-2 pr-3 font-medium text-cyan-100">{dataset.run.runId}</td>
                    <td className="py-2 pr-3">{dataset.run.label}</td>
                    <td className="py-2 pr-3">{dataset.run.environment}</td>
                    <td className="py-2 pr-3">{dataset.run.totalTimesteps}</td>
                    <td className="py-2 pr-3">{dataset.run.backend}</td>
                    <td className="py-2 pr-3">{dataset.run.deterministic ? "yes" : "no"}</td>
                    <td className="py-2 pr-3">{dataset.run.artifactStatus}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </PanelBody>
        </Panel>
      </div>
    </main>
  );
}
