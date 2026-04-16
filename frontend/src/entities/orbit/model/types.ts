export type Vector3 = [number, number, number];

export interface OrbitPathPoint {
  timestep: number;
  missionTimeS: number;
  positionKm: Vector3;
  velocityKmS: Vector3;
  orbitalErrorKm: number;
  controlMagnitude: number;
  reward: number;
}
