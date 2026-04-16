import { useEffect, useState } from "react";

import { orbitalApi } from "@/shared/api";
import type { ReplayPayloadDto } from "@/shared/api/generated/orbital-api";

interface UseReplayQueryState {
  replay: ReplayPayloadDto | null;
  loading: boolean;
  error: string | null;
}

export function useReplayQuery(runId: string | null, downsample = 1200): UseReplayQueryState {
  const [state, setState] = useState<UseReplayQueryState>({
    replay: null,
    loading: false,
    error: null,
  });

  useEffect(() => {
    if (!runId) {
      setState({ replay: null, loading: false, error: null });
      return;
    }

    let cancelled = false;
    setState((previous) => ({ ...previous, loading: true, error: null }));

    orbitalApi
      .getRunReplay(runId, { downsample })
      .then((result) => {
        if (cancelled) {
          return;
        }
        setState({ replay: result.data, loading: false, error: null });
      })
      .catch((error: unknown) => {
        if (cancelled) {
          return;
        }
        setState({
          replay: null,
          loading: false,
          error: error instanceof Error ? error.message : "Unable to load replay",
        });
      });

    return () => {
      cancelled = true;
    };
  }, [downsample, runId]);

  return state;
}
