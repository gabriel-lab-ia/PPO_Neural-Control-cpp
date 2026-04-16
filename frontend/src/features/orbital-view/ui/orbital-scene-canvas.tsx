"use client";

import { Float, Line, OrbitControls, Stars } from "@react-three/drei";
import { Canvas, useFrame } from "@react-three/fiber";
import { useEffect, useMemo, useRef, useState } from "react";
import * as THREE from "three";

import type { OrbitPathPoint } from "@/entities/orbit/model/types";
import { createProceduralEarthTextures } from "@/features/orbital-view/lib/procedural-earth-texture";

interface OrbitalSceneCanvasProps {
  orbitPath: OrbitPathPoint[];
  currentPoint: OrbitPathPoint;
}

const SCENE_SCALE = 1 / 1900;
const EARTH_RADIUS = 3.12;

const LOCAL_TEXTURES = {
  day: "/textures/earth/earth_day_4k.jpg",
  night: "/textures/earth/earth_night_4k.jpg",
  clouds: "/textures/earth/earth_clouds_4k.png",
  normal: "/textures/earth/earth_normal_4k.jpg",
  specular: "/textures/earth/earth_specular_4k.jpg",
} as const;

interface EarthTextureSet {
  day: THREE.Texture;
  night: THREE.Texture;
  clouds: THREE.Texture;
  normal: THREE.Texture | null;
  specular: THREE.Texture | null;
  source: "procedural" | "textured";
}

function configureTexture(texture: THREE.Texture, srgb: boolean): THREE.Texture {
  texture.colorSpace = srgb ? THREE.SRGBColorSpace : THREE.NoColorSpace;
  texture.wrapS = THREE.RepeatWrapping;
  texture.wrapT = THREE.ClampToEdgeWrapping;
  texture.anisotropy = 8;
  texture.needsUpdate = true;
  return texture;
}

function toSceneCoordinates(point: OrbitPathPoint): [number, number, number] {
  const [x, y, z] = point.positionKm;
  return [x * SCENE_SCALE, z * SCENE_SCALE, y * SCENE_SCALE];
}

function useEarthTextures(): EarthTextureSet {
  const fallback = useMemo(() => createProceduralEarthTextures(), []);
  const [textures, setTextures] = useState<EarthTextureSet>(() => ({
    ...fallback,
    normal: null,
    specular: null,
    source: "procedural",
  }));

  useEffect(() => {
    let active = true;
    const loader = new THREE.TextureLoader();

    const loadTexture = async (url: string, srgb: boolean): Promise<THREE.Texture | null> => {
      try {
        const texture = await loader.loadAsync(url);
        if (!active) {
          texture.dispose();
          return null;
        }
        return configureTexture(texture, srgb);
      } catch {
        return null;
      }
    };

    void (async () => {
      const [day, night, clouds, normal, specular] = await Promise.all([
        loadTexture(LOCAL_TEXTURES.day, true),
        loadTexture(LOCAL_TEXTURES.night, true),
        loadTexture(LOCAL_TEXTURES.clouds, true),
        loadTexture(LOCAL_TEXTURES.normal, false),
        loadTexture(LOCAL_TEXTURES.specular, false),
      ]);

      if (!active) {
        return;
      }

      if (day && night && clouds) {
        setTextures({ day, night, clouds, normal, specular, source: "textured" });
      }
    })();

    return () => {
      active = false;
    };
  }, [fallback]);

  return textures;
}

function AtmosphereGlow(): JSX.Element {
  const materialRef = useRef<THREE.ShaderMaterial>(null);

  useFrame(({ camera }) => {
    if (!materialRef.current) {
      return;
    }

    const viewVector = materialRef.current.uniforms.uViewVector.value as THREE.Vector3;
    viewVector.copy(camera.position).normalize();
  });

  return (
    <mesh>
      <sphereGeometry args={[EARTH_RADIUS * 1.05, 128, 128]} />
      <shaderMaterial
        ref={materialRef}
        transparent
        depthWrite={false}
        blending={THREE.AdditiveBlending}
        side={THREE.BackSide}
        uniforms={{
          uGlowColor: { value: new THREE.Color("#4abfff") },
          uViewVector: { value: new THREE.Vector3(0, 0, 8) },
        }}
        vertexShader={`
          uniform vec3 uViewVector;
          varying float vIntensity;
          void main() {
            vec3 vNormal = normalize(normalMatrix * normal);
            vec3 viewDir = normalize(normalMatrix * uViewVector);
            vIntensity = pow(max(0.0, 0.74 - dot(vNormal, viewDir)), 2.6);
            gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
          }
        `}
        fragmentShader={`
          uniform vec3 uGlowColor;
          varying float vIntensity;
          void main() {
            gl_FragColor = vec4(uGlowColor * vIntensity, vIntensity * 0.72);
          }
        `}
      />
    </mesh>
  );
}

function EarthScene({ orbitPath, currentPoint }: OrbitalSceneCanvasProps): JSX.Element {
  const earthRef = useRef<THREE.Mesh>(null);
  const cloudRef = useRef<THREE.Mesh>(null);
  const satelliteRef = useRef<THREE.Mesh>(null);

  const trajectoryPoints = useMemo(() => orbitPath.map(toSceneCoordinates), [orbitPath]);
  const satellitePosition = toSceneCoordinates(currentPoint);
  const textures = useEarthTextures();

  useFrame((state, delta) => {
    if (earthRef.current) {
      earthRef.current.rotation.y += delta * 0.035;
    }

    if (cloudRef.current) {
      cloudRef.current.rotation.y += delta * 0.044;
    }

    if (satelliteRef.current) {
      satelliteRef.current.rotation.y += delta * 1.5;
      const pulse = 1 + Math.sin(state.clock.elapsedTime * 4.2) * 0.08;
      satelliteRef.current.scale.setScalar(pulse);
    }
  });

  return (
    <>
      <color attach="background" args={["#020202"]} />
      <ambientLight intensity={0.08} />
      <hemisphereLight intensity={0.2} color="#8ec7ff" groundColor="#02040a" />

      <directionalLight position={[10, 3.8, 8]} intensity={2.15} color="#ffffff" />
      <directionalLight position={[-11, -4, -8]} intensity={0.1} color="#3764a8" />

      <mesh ref={earthRef} rotation={[0, Math.PI * 0.24, 0]}>
        <sphereGeometry args={[EARTH_RADIUS, 128, 128]} />
        <meshStandardMaterial
          map={textures.day}
          normalMap={textures.normal ?? undefined}
          emissiveMap={textures.night}
          emissive="#ffad5b"
          emissiveIntensity={0.5}
          metalness={0.02}
          roughness={0.88}
          aoMapIntensity={0.45}
          roughnessMap={textures.specular ?? undefined}
        />
      </mesh>

      <mesh ref={cloudRef} rotation={[0, Math.PI * 0.2, 0]}>
        <sphereGeometry args={[EARTH_RADIUS * 1.007, 128, 128]} />
        <meshStandardMaterial
          map={textures.clouds}
          transparent
          opacity={textures.source === "textured" ? 0.36 : 0.24}
          depthWrite={false}
          roughness={1}
          metalness={0}
        />
      </mesh>

      <AtmosphereGlow />

      {trajectoryPoints.length > 1 ? (
        <>
          <Line points={trajectoryPoints} color="#67e8f9" lineWidth={1.15} />
          <Line points={trajectoryPoints} color="#22d3ee" lineWidth={3.2} transparent opacity={0.22} />
        </>
      ) : null}

      <Float speed={1.2} rotationIntensity={0.4} floatIntensity={0.1}>
        <mesh ref={satelliteRef} position={satellitePosition}>
          <icosahedronGeometry args={[0.094, 1]} />
          <meshStandardMaterial color="#f5faff" emissive="#61ebff" emissiveIntensity={1.8} />
        </mesh>
      </Float>

      <Line points={[[0, 0, 0], satellitePosition]} color="#1f4f79" lineWidth={0.8} dashed dashSize={0.15} gapSize={0.08} />

      <mesh rotation={[-Math.PI / 2, 0, 0]}>
        <ringGeometry args={[EARTH_RADIUS + 0.45, EARTH_RADIUS + 0.48, 256]} />
        <meshBasicMaterial color="#22557b" transparent opacity={0.4} side={THREE.DoubleSide} />
      </mesh>

      <Stars radius={220} depth={65} count={1900} factor={3.2} saturation={0} fade speed={0.08} />

      <OrbitControls
        enablePan
        enableDamping
        dampingFactor={0.06}
        minDistance={4.9}
        maxDistance={17}
        autoRotate
        autoRotateSpeed={0.16}
      />
    </>
  );
}

export function OrbitalSceneCanvas({ orbitPath, currentPoint }: OrbitalSceneCanvasProps): JSX.Element {
  return (
    <Canvas
      camera={{ position: [0, 4.8, 10.4], fov: 36 }}
      dpr={[1, 1.6]}
      gl={{ antialias: true, powerPreference: "high-performance" }}
    >
      <EarthScene orbitPath={orbitPath} currentPoint={currentPoint} />
    </Canvas>
  );
}
