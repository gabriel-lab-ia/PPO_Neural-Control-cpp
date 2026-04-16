export function formatNumber(value: number, digits = 3): string {
  return value.toFixed(digits);
}

export function formatTimestamp(iso: string): string {
  return new Date(iso).toLocaleString();
}

export function formatBoolean(value: boolean): string {
  return value ? "yes" : "no";
}
