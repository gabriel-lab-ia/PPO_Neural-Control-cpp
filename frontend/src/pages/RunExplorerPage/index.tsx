import { Link } from "react-router-dom";

import { useRunsQuery } from "@/shared/api/hooks/use-runs-query";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

export function RunExplorerPage(): JSX.Element {
  const { runs, loading, error } = useRunsQuery(300);

  return (
    <main className="mission-shell">
      <div className="mission-shell__gradient" />
      <div className="mission-shell__content space-y-4">
        <header className="hud-panel glow-border flex flex-wrap items-center justify-between gap-3 rounded-xl border border-cyan-200/15 px-4 py-3">
          <div>
            <p className="text-xs uppercase tracking-[0.24em] text-cyan-300/85">Run Explorer</p>
            <h1 className="text-2xl font-semibold uppercase tracking-[0.1em] text-slate-50">Replay Dataset Registry</h1>
          </div>

          <Link
            to="/"
            className="rounded-lg border border-cyan-200/25 bg-black/55 px-3 py-2 text-sm font-medium text-slate-100 transition hover:border-cyan-300/65 hover:text-cyan-200"
          >
            Back to Mission Replay
          </Link>
        </header>

        {error ? (
          <div className="rounded-xl border border-amber-500/40 bg-amber-500/10 px-3 py-2 text-sm text-amber-200">
            Backend unavailable ({error}). Showing empty registry.
          </div>
        ) : null}

        <Panel>
          <PanelHeader>
            <PanelTitle>Available Runs</PanelTitle>
          </PanelHeader>
          <PanelBody className="overflow-x-auto">
            <table className="w-full min-w-[920px] border-collapse text-left text-sm text-slate-200">
              <thead>
                <tr className="border-b border-cyan-200/15 text-[11px] uppercase tracking-[0.16em] text-slate-400">
                  <th className="py-2 pr-3">run_id</th>
                  <th className="py-2 pr-3">mode</th>
                  <th className="py-2 pr-3">environment</th>
                  <th className="py-2 pr-3">seed</th>
                  <th className="py-2 pr-3">status</th>
                  <th className="py-2 pr-3">started_at</th>
                  <th className="py-2 pr-3">artifact_dir</th>
                </tr>
              </thead>
              <tbody>
                {runs.map((run) => (
                  <tr key={run.run_id} className="border-b border-cyan-200/10 last:border-b-0">
                    <td className="mono py-2 pr-3 font-medium text-cyan-100">{run.run_id}</td>
                    <td className="py-2 pr-3">{run.mode}</td>
                    <td className="py-2 pr-3">{run.environment}</td>
                    <td className="mono py-2 pr-3">{run.seed}</td>
                    <td className="py-2 pr-3">{run.status}</td>
                    <td className="py-2 pr-3">{run.started_at}</td>
                    <td className="mono py-2 pr-3">{run.artifact_dir}</td>
                  </tr>
                ))}

                {!loading && runs.length === 0 ? (
                  <tr>
                    <td className="py-3 text-slate-400" colSpan={7}>
                      No runs found.
                    </td>
                  </tr>
                ) : null}
              </tbody>
            </table>
          </PanelBody>
        </Panel>
      </div>
    </main>
  );
}
