import { Gauge, Navigation, Orbit } from "lucide-react";

import type { OrbitPathPoint } from "@/entities/orbit/model/types";
import type { ReplayFrame } from "@/entities/replay/model/types";
import { OrbitalSceneCanvas } from "@/features/orbital-view/ui/orbital-scene-canvas";
import { formatNumber } from "@/shared/lib/format";
import { Panel, PanelBody, PanelHeader, PanelTitle } from "@/shared/ui/panel";

interface OrbitalCanvasWidgetProps {
  frame: ReplayFrame;
  orbitPath: OrbitPathPoint[];
}

export function OrbitalCanvasWidget({ frame, orbitPath }: OrbitalCanvasWidgetProps): JSX.Element {
  return (
    <Panel className="h-full overflow-hidden">
      <PanelHeader className="flex flex-row flex-wrap items-center justify-between gap-3">
        <PanelTitle>Orbital Viewport</PanelTitle>

        <div className="flex flex-wrap items-center gap-2 text-[10px] uppercase tracking-[0.16em]">
          <span className="inline-flex items-center gap-1.5 rounded-full border border-cyan-300/35 bg-cyan-300/12 px-2 py-1 text-cyan-100">
            <Orbit className="h-3.5 w-3.5" />
            Path {orbitPath.length}
          </span>
          <span className="inline-flex items-center gap-1.5 rounded-full border border-sky-300/30 bg-sky-300/10 px-2 py-1 text-sky-100">
            <Navigation className="h-3.5 w-3.5" />
            Error {formatNumber(frame.telemetry.orbitalErrorKm, 3)} km
          </span>
          <span className="inline-flex items-center gap-1.5 rounded-full border border-violet-300/30 bg-violet-300/10 px-2 py-1 text-violet-100">
            <Gauge className="h-3.5 w-3.5" />
            Reward {formatNumber(frame.telemetry.reward, 3)}
          </span>
        </div>
      </PanelHeader>

      <PanelBody className="relative p-0">
        <div className="relative h-[620px] w-full overflow-hidden bg-black lg:h-[680px]">
          <OrbitalSceneCanvas orbitPath={orbitPath} currentPoint={frame.orbit} />
          <div className="scanline-overlay" />
        </div>
      </PanelBody>
    </Panel>
  );
}
