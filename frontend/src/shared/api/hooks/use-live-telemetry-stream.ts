import { useEffect, useMemo, useState } from "react";

import { backendWsBaseUrl } from "@/shared/api";
import type {
  TelemetrySampleDto,
  WsMessage,
  WsReplayChunk,
  WsTelemetrySample,
} from "@/shared/api/generated/orbital-api";

interface UseLiveTelemetryStreamOptions {
  runId: string | null;
  enabled: boolean;
  maxSamples?: number;
}

interface UseLiveTelemetryStreamState {
  samples: TelemetrySampleDto[];
  connected: boolean;
  error: string | null;
}

function isTelemetryMessage(message: WsMessage): message is WsTelemetrySample {
  return message.type === "telemetry.sample";
}

function isReplayChunkMessage(message: WsMessage): message is WsReplayChunk {
  return message.type === "replay.chunk";
}

export function useLiveTelemetryStream(options: UseLiveTelemetryStreamOptions): UseLiveTelemetryStreamState {
  const { runId, enabled, maxSamples = 2000 } = options;

  const [samples, setSamples] = useState<TelemetrySampleDto[]>([]);
  const [connected, setConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    if (!enabled || !runId) {
      setConnected(false);
      setError(null);
      setSamples([]);
      return;
    }

    const path = `${backendWsBaseUrl}/ws/runs/${encodeURIComponent(runId)}/stream`;
    const socket = new WebSocket(path);
    setConnected(false);
    setError(null);
    setSamples([]);

    socket.onopen = () => {
      setConnected(true);
    };

    socket.onerror = () => {
      setError("WebSocket stream error");
    };

    socket.onclose = () => {
      setConnected(false);
    };

    socket.onmessage = (event) => {
      try {
        const message = JSON.parse(String(event.data)) as WsMessage;

        if (isTelemetryMessage(message)) {
          setSamples((previous) => {
            const next = [...previous, message.payload];
            return next.length > maxSamples ? next.slice(next.length - maxSamples) : next;
          });
          return;
        }

        if (isReplayChunkMessage(message)) {
          setSamples((previous) => {
            const next = [...previous, ...message.payload.samples];
            return next.length > maxSamples ? next.slice(next.length - maxSamples) : next;
          });
        }
      } catch {
        setError("Invalid stream message");
      }
    };

    return () => {
      socket.close();
    };
  }, [enabled, maxSamples, runId]);

  return useMemo(
    () => ({
      samples,
      connected,
      error,
    }),
    [connected, error, samples],
  );
}
