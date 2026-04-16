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
  backend: string;
  deterministic: boolean;
  positionKm: Vector3;
  velocityKmS: Vector3;
  controlVector: Vector3;
  terminated?: boolean;
  truncated?: boolean;
  timestampIso?: string;
}
