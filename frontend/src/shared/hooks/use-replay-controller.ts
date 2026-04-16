"use client";

import { useCallback, useEffect, useMemo, useState } from "react";

import { DEFAULT_FRAME_RATE, DEFAULT_REPLAY_SPEED } from "@/shared/config/replay";
import { clamp } from "@/shared/lib/math";

interface ReplayControllerOptions {
  totalFrames: number;
  initialFrame?: number;
  initialPlaying?: boolean;
  initialSpeed?: number;
}

export interface ReplayControllerState {
  frameIndex: number;
  isPlaying: boolean;
  speed: number;
  frameCount: number;
  progress: number;
  setFrameIndex: (frameIndex: number) => void;
  setPlaying: (value: boolean) => void;
  togglePlaying: () => void;
  setSpeed: (value: number) => void;
  reset: () => void;
}

export function useReplayController(options: ReplayControllerOptions): ReplayControllerState {
  const {
    totalFrames,
    initialFrame = 0,
    initialPlaying = false,
    initialSpeed = DEFAULT_REPLAY_SPEED,
  } = options;

  const frameCount = Math.max(1, totalFrames);
  const [frameIndex, setFrameIndexInternal] = useState(() => clamp(initialFrame, 0, frameCount - 1));
  const [isPlaying, setPlaying] = useState(initialPlaying);
  const [speed, setSpeedInternal] = useState(initialSpeed);

  useEffect(() => {
    setFrameIndexInternal((previous) => clamp(previous, 0, frameCount - 1));
  }, [frameCount]);

  useEffect(() => {
    if (!isPlaying) {
      return;
    }

    const intervalMs = Math.max(12, Math.round(1000 / (DEFAULT_FRAME_RATE * speed)));
    const timer = window.setInterval(() => {
      setFrameIndexInternal((previous) => {
        if (previous >= frameCount - 1) {
          return 0;
        }
        return previous + 1;
      });
    }, intervalMs);

    return () => {
      window.clearInterval(timer);
    };
  }, [frameCount, isPlaying, speed]);

  const setFrameIndex = useCallback(
    (value: number) => {
      setFrameIndexInternal(clamp(value, 0, frameCount - 1));
    },
    [frameCount],
  );

  const togglePlaying = useCallback(() => {
    setPlaying((previous) => !previous);
  }, []);

  const setSpeed = useCallback((value: number) => {
    setSpeedInternal(value > 0 ? value : DEFAULT_REPLAY_SPEED);
  }, []);

  const reset = useCallback(() => {
    setFrameIndexInternal(0);
    setPlaying(false);
  }, []);

  return useMemo(
    () => ({
      frameIndex,
      isPlaying,
      speed,
      frameCount,
      progress: frameCount <= 1 ? 0 : frameIndex / (frameCount - 1),
      setFrameIndex,
      setPlaying,
      togglePlaying,
      setSpeed,
      reset,
    }),
    [frameCount, frameIndex, isPlaying, reset, setFrameIndex, setSpeed, speed, togglePlaying],
  );
}
