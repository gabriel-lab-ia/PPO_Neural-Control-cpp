import type { ApiErrorEnvelope } from "@/shared/api/generated/orbital-api";

export class ApiRequestError extends Error {
  public readonly status: number;
  public readonly code: string;
  public readonly path: string;

  constructor(message: string, status: number, code = "request_failed", path = "") {
    super(message);
    this.name = "ApiRequestError";
    this.status = status;
    this.code = code;
    this.path = path;
  }
}

function isApiErrorEnvelope(value: unknown): value is ApiErrorEnvelope {
  if (typeof value !== "object" || value === null) {
    return false;
  }
  const maybeError = value as Partial<ApiErrorEnvelope>;
  return maybeError.status === "error" && typeof maybeError.error?.message === "string";
}

export async function requestJson<T>(input: string, init?: RequestInit): Promise<T> {
  const response = await fetch(input, {
    ...init,
    headers: {
      Accept: "application/json",
      ...(init?.body ? { "Content-Type": "application/json" } : {}),
      ...(init?.headers ?? {}),
    },
  });

  const body = (await response.json()) as unknown;

  if (!response.ok) {
    if (isApiErrorEnvelope(body)) {
      throw new ApiRequestError(body.error.message, response.status, body.error.code, body.error.path);
    }

    throw new ApiRequestError(`Request failed with status ${response.status}`, response.status);
  }

  return body as T;
}
