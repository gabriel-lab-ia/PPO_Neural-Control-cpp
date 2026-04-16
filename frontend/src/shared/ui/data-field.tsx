import type { ReactNode } from "react";

interface DataFieldProps {
  label: string;
  value: ReactNode;
}

export function DataField({ label, value }: DataFieldProps): JSX.Element {
  return (
    <div className="flex items-center justify-between gap-3 border-b border-cyan-100/10 py-2.5 last:border-b-0">
      <span className="hud-label">{label}</span>
      <span className="hud-value text-right text-sm font-medium">{value}</span>
    </div>
  );
}
