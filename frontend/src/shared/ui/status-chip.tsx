import type { ReactNode } from "react";

import { cn } from "@/shared/ui/utils";

interface StatusChipProps {
  children: ReactNode;
  tone?: "default" | "ok" | "warning";
}

export function StatusChip({ children, tone = "default" }: StatusChipProps): JSX.Element {
  return (
    <span
      className={cn(
        "inline-flex items-center rounded-full border px-3 py-1 text-[11px] font-semibold uppercase tracking-[0.14em]",
        tone === "ok" && "border-emerald-400/45 bg-emerald-400/12 text-emerald-200",
        tone === "warning" && "border-amber-400/45 bg-amber-400/12 text-amber-200",
        tone === "default" && "border-cyan-400/45 bg-cyan-400/12 text-cyan-100",
      )}
    >
      {children}
    </span>
  );
}
