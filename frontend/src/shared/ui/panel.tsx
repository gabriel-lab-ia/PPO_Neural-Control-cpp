import type { HTMLAttributes } from "react";

import { cn } from "@/shared/ui/utils";

export function Panel({ className, ...props }: HTMLAttributes<HTMLDivElement>): JSX.Element {
  return (
    <section
      className={cn(
        "hud-panel glow-border rounded-2xl border border-cyan-200/15 bg-[rgba(8,11,18,0.66)] shadow-[0_18px_60px_rgba(0,0,0,0.6)] backdrop-blur-md",
        className,
      )}
      {...props}
    />
  );
}

export function PanelHeader({ className, ...props }: HTMLAttributes<HTMLDivElement>): JSX.Element {
  return (
    <header
      className={cn(
        "border-b border-cyan-200/10 px-4 py-3 sm:px-5 sm:py-4",
        "bg-gradient-to-r from-cyan-300/[0.03] via-transparent to-violet-400/[0.04]",
        className,
      )}
      {...props}
    />
  );
}

export function PanelBody({ className, ...props }: HTMLAttributes<HTMLDivElement>): JSX.Element {
  return <div className={cn("px-4 py-4 sm:px-5", className)} {...props} />;
}

export function PanelTitle({ className, ...props }: HTMLAttributes<HTMLHeadingElement>): JSX.Element {
  return (
    <h3 className={cn("text-[11px] font-semibold uppercase tracking-[0.22em] text-cyan-100/95", className)} {...props} />
  );
}
