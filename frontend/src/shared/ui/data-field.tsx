import type { ReactNode } from "react";

interface DataFieldProps {
  label: string;
  value: ReactNode;
}

export function DataField({ label, value }: DataFieldProps): JSX.Element {
  return (
    <div className="flex items-center justify-between gap-4 border-b border-slate-800/70 py-2.5 last:border-b-0">
      <span className="text-[11px] uppercase tracking-[0.16em] text-slate-400">{label}</span>
      <span className="text-sm font-medium text-slate-100">{value}</span>
    </div>
  );
}
