"use client";

import { Line, OrbitControls, Stars } from "@react-three/drei";
import { Canvas } from "@react-three/fiber";
import { useMemo } from "react";
import * as THREE from "three";

import type { OrbitPathPoint } from "@/entities/orbit/model/types";
import { createProceduralEarthTextures } from "@/features/orbital-view/lib/procedural-earth-texture";

interface OrbitalSceneCanvasProps {
  orbitPath: OrbitPathPoint[];
  currentPoint: OrbitPathPoint;
}

const SCENE_SCALE = 1 / 1900;

function toSceneCoordinates(point: OrbitPathPoint): [number, number, number] {
  const [x, y, z] = point.positionKm;
  return [x * SCENE_SCALE, z * SCENE_SCALE, y * SCENE_SCALE];
}

function DirectionalGlow(): JSX.Element {
  return (
    <mesh position={[0, 0, 0]}>
      <sphereGeometry args={[3.55, 64, 64]} />
      <meshBasicMaterial color="#6ab8ff" transparent opacity={0.05} blending={THREE.AdditiveBlending} depthWrite={false} />
    </mesh>
  );
}

export function OrbitalSceneCanvas({ orbitPath, currentPoint }: OrbitalSceneCanvasProps): JSX.Element {
  const trajectoryPoints = useMemo(() => orbitPath.map(toSceneCoordinates), [orbitPath]);
  const satellitePosition = toSceneCoordinates(currentPoint);

  const textures = useMemo(() => createProceduralEarthTextures(), []);

  return (
    <Canvas camera={{ position: [0, 4.8, 10.2], fov: 37 }}>
      <color attach="background" args={["#02060f"]} />
      <ambientLight intensity={0.12} />
      <directionalLight position={[8, 3, 6]} intensity={1.8} color="#e9f6ff" />
      <directionalLight position={[-8, -3, -6]} intensity={0.18} color="#4b6997" />
      <pointLight position={[0, 0, 0]} intensity={0.16} color="#4aa2ff" />

      <mesh rotation={[0, Math.PI * 0.16, 0]}>
        <sphereGeometry args={[3.12, 128, 128]} />
        <meshStandardMaterial
          map={textures.day}
          emissiveMap={textures.night}
          emissive="#ff7a1d"
          emissiveIntensity={0.32}
          roughness={0.92}
          metalness={0.02}
        />
      </mesh>

      <mesh rotation={[0, Math.PI * 0.14, 0]}>
        <sphereGeometry args={[3.18, 96, 96]} />
        <meshStandardMaterial
          map={textures.clouds}
          transparent
          opacity={0.28}
          depthWrite={false}
          roughness={1}
          metalness={0}
        />
      </mesh>

      <mesh>
        <sphereGeometry args={[3.29, 96, 96]} />
        <meshBasicMaterial color="#4ea9ff" transparent opacity={0.09} blending={THREE.AdditiveBlending} depthWrite={false} />
      </mesh>

      <DirectionalGlow />

      {trajectoryPoints.length > 1 ? <Line points={trajectoryPoints} color="#47c2ff" lineWidth={1.1} /> : null}

      <mesh position={satellitePosition}>
        <icosahedronGeometry args={[0.092, 1]} />
        <meshStandardMaterial color="#d9f5ff" emissive="#66d9ff" emissiveIntensity={1.4} />
      </mesh>

      <Line points={[[0, 0, 0], satellitePosition]} color="#2d7eb8" lineWidth={0.6} dashed dashSize={0.2} gapSize={0.15} />

      <mesh>
        <ringGeometry args={[3.6, 3.64, 256]} />
        <meshBasicMaterial color="#284868" transparent opacity={0.5} side={THREE.DoubleSide} />
      </mesh>

      <Stars radius={180} depth={120} count={2600} factor={5} saturation={0} fade speed={0.15} />
      <OrbitControls enablePan enableDamping dampingFactor={0.04} minDistance={4.8} maxDistance={16} autoRotate autoRotateSpeed={0.08} />
    </Canvas>
  );
}
