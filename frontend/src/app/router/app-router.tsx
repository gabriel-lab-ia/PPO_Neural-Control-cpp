import { BrowserRouter, Navigate, Route, Routes } from "react-router-dom";

import { MissionReplayPage } from "@/pages/MissionReplayPage";
import { RunExplorerPage } from "@/pages/RunExplorerPage";

export function AppRouter(): JSX.Element {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<MissionReplayPage />} />
        <Route path="/runs" element={<RunExplorerPage />} />
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </BrowserRouter>
  );
}
