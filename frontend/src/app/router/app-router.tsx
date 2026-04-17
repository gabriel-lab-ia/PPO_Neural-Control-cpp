import { lazy, Suspense } from "react";

import { BrowserRouter, Navigate, Route, Routes } from "react-router-dom";

const MissionReplayPage = lazy(async () => {
  const module = await import("@/pages/MissionReplayPage");
  return { default: module.MissionReplayPage };
});

const RunExplorerPage = lazy(async () => {
  const module = await import("@/pages/RunExplorerPage");
  return { default: module.RunExplorerPage };
});

export function AppRouter(): JSX.Element {
  return (
    <BrowserRouter>
      <Suspense
        fallback={
          <main className="flex min-h-screen items-center justify-center bg-black text-[11px] uppercase tracking-[0.2em] text-cyan-100/70">
            Loading mission console...
          </main>
        }
      >
        <Routes>
          <Route path="/" element={<MissionReplayPage />} />
          <Route path="/runs" element={<RunExplorerPage />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </Suspense>
    </BrowserRouter>
  );
}
