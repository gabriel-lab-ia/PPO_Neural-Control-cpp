import type { Vector3 } from "@/entities/orbit/model/types";

export function magnitude3(value: Vector3): number {
  const [x, y, z] = value;
  return Math.sqrt(x * x + y * y + z * z);
}

export function clamp(value: number, min: number, max: number): number {
  return Math.min(Math.max(value, min), max);
}

export function lerp(a: number, b: number, t: number): number {
  return a + (b - a) * t;
}
