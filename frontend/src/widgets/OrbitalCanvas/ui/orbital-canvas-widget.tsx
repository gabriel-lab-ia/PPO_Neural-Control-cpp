import type { ReplayFrame } from "@/entities/replay/model/types";
import type { OrbitPathPoint } from "@/entities/orbit/model/types";
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
      <PanelHeader className="flex flex-row items-center justify-between gap-4">
        <PanelTitle>Orbital Viewport</PanelTitle>
        <div className="text-xs uppercase tracking-[0.14em] text-slate-400">
          error {formatNumber(frame.telemetry.orbitalErrorKm, 3)} km
        </div>
      </PanelHeader>
      <PanelBody className="p-0">
        <div className="h-[520px] w-full bg-gradient-to-b from-[#030815] via-[#051229] to-[#03050d]">
          <OrbitalSceneCanvas orbitPath={orbitPath} currentPoint={frame.orbit} />
        </div>
      </PanelBody>
    </Panel>
  );
}
