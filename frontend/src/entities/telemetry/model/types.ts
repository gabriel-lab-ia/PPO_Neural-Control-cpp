import type { Vector3 } from "@/entities/orbit/model/types";

export interface TelemetrySample {
  runId: string;
  environment: string;
  timestep: number;
  missionTimeS: number;
  reward: number;
  controlMagnitude: number;
  orbitalErrorKm: number;
  velocityMagnitudeKmS: number;
  policyStd: number;
  backend: "libtorch_cpu";
  deterministic: boolean;
  positionKm: Vector3;
  controlVector: Vector3;
}
