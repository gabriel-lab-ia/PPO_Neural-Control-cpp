import { createOrbitalApiClient } from "@/shared/api/orbital-api-client";

const backendHttp = import.meta.env.VITE_BACKEND_HTTP as string | undefined;
const resolvedBaseUrl = backendHttp && backendHttp.trim().length > 0 ? backendHttp : "http://localhost:8080";

export const orbitalApi = createOrbitalApiClient(resolvedBaseUrl);

export const backendWsBaseUrl = (() => {
  const fromEnv = import.meta.env.VITE_BACKEND_WS as string | undefined;
  if (fromEnv && fromEnv.trim().length > 0) {
    return fromEnv.replace(/\/$/, "");
  }

  if (resolvedBaseUrl.startsWith("https://")) {
    return resolvedBaseUrl.replace("https://", "wss://");
  }
  if (resolvedBaseUrl.startsWith("http://")) {
    return resolvedBaseUrl.replace("http://", "ws://");
  }
  return resolvedBaseUrl;
})();
