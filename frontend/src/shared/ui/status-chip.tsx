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
        "inline-flex items-center gap-2 rounded-full border px-3 py-1 text-[10px] font-semibold uppercase tracking-[0.16em]",
        tone === "ok" && "border-emerald-300/45 bg-emerald-400/14 text-emerald-100",
        tone === "warning" && "border-amber-300/50 bg-amber-400/14 text-amber-100",
        tone === "default" && "border-cyan-300/45 bg-cyan-300/14 text-cyan-100",
      )}
    >
      <span
        className={cn(
          "h-1.5 w-1.5 rounded-full",
          tone === "ok" && "bg-emerald-300",
          tone === "warning" && "bg-amber-300",
          tone === "default" && "bg-cyan-300",
        )}
      />
      {children}
    </span>
  );
}
