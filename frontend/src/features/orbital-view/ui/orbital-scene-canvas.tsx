"use client";

import { Line, OrbitControls, Stars } from "@react-three/drei";
import { Canvas } from "@react-three/fiber";
import { useMemo } from "react";

import type { OrbitPathPoint } from "@/entities/orbit/model/types";

interface OrbitalSceneCanvasProps {
  orbitPath: OrbitPathPoint[];
  currentPoint: OrbitPathPoint;
}

const SCENE_SCALE = 1 / 2000;

function toSceneCoordinates(point: OrbitPathPoint): [number, number, number] {
  const [x, y, z] = point.positionKm;
  return [x * SCENE_SCALE, z * SCENE_SCALE, y * SCENE_SCALE];
}

export function OrbitalSceneCanvas({ orbitPath, currentPoint }: OrbitalSceneCanvasProps): JSX.Element {
  const trajectoryPoints = useMemo(() => orbitPath.map(toSceneCoordinates), [orbitPath]);
  const satellitePosition = toSceneCoordinates(currentPoint);

  return (
    <Canvas camera={{ position: [0, 5.2, 9.1], fov: 42 }}>
      <color attach="background" args={["#030814"]} />
      <ambientLight intensity={0.38} />
      <directionalLight position={[4, 6, 6]} intensity={1.2} color="#7bc7ff" />
      <pointLight position={[-6, -2, -4]} intensity={0.35} color="#4dd9d9" />

      <mesh>
        <sphereGeometry args={[3.18, 72, 72]} />
        <meshStandardMaterial color="#0d2a49" roughness={0.78} metalness={0.1} emissive="#0b2848" emissiveIntensity={0.35} />
      </mesh>

      <mesh>
        <sphereGeometry args={[3.24, 64, 64]} />
        <meshStandardMaterial color="#2c638f" transparent opacity={0.14} roughness={0.92} />
      </mesh>

      {trajectoryPoints.length > 1 ? <Line points={trajectoryPoints} color="#5fd1ff" lineWidth={1.5} /> : null}

      <mesh position={satellitePosition}>
        <sphereGeometry args={[0.09, 24, 24]} />
        <meshStandardMaterial color="#8ed6ff" emissive="#4ef0ff" emissiveIntensity={1.2} />
      </mesh>

      <Line points={[[0, 0, 0], satellitePosition]} color="#2e6fa3" lineWidth={0.6} dashed dashSize={0.2} gapSize={0.15} />

      <Stars radius={120} depth={70} count={1900} factor={4} saturation={0} fade speed={0.35} />
      <OrbitControls enablePan={false} minDistance={5.4} maxDistance={14} autoRotate autoRotateSpeed={0.16} />
    </Canvas>
  );
}
