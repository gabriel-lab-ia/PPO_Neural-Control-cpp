export interface EpisodeSummary {
  episodeId: string;
  stepCount: number;
  cumulativeReward: number;
  terminalReason: "timeout" | "goal" | "constraint";
}
