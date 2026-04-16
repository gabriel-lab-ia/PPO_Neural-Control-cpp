import { useEffect, useState } from "react";

import type { RunDto } from "@/shared/api/generated/orbital-api";
import { orbitalApi } from "@/shared/api";

interface UseRunsQueryState {
  runs: RunDto[];
  loading: boolean;
  error: string | null;
}

export function useRunsQuery(limit = 100): UseRunsQueryState {
  const [state, setState] = useState<UseRunsQueryState>({
    runs: [],
    loading: true,
    error: null,
  });

  useEffect(() => {
    let cancelled = false;

    orbitalApi
      .listRuns({ limit, offset: 0 })
      .then((result) => {
        if (cancelled) {
          return;
        }
        setState({ runs: result.data.items, loading: false, error: null });
      })
      .catch((error: unknown) => {
        if (cancelled) {
          return;
        }
        setState({
          runs: [],
          loading: false,
          error: error instanceof Error ? error.message : "Unable to load runs",
        });
      });

    return () => {
      cancelled = true;
    };
  }, [limit]);

  return state;
}
