export interface RunSummary {
  runId: string;
  label: string;
  environment: string;
  backend: "libtorch_cpu";
  deterministic: boolean;
  totalTimesteps: number;
  status: "ok" | "warning";
  artifactStatus: "complete" | "partial";
  startedAtIso: string;
}

export interface BenchmarkSummary {
  totalTimesteps: number;
  meanReward: number;
  deterministic: boolean;
  backend: "libtorch_cpu";
  artifactStatus: "complete" | "partial";
}
