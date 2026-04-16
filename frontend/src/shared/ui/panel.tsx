import type { HTMLAttributes } from "react";

import { cn } from "@/shared/ui/utils";

export function Panel({ className, ...props }: HTMLAttributes<HTMLDivElement>): JSX.Element {
  return (
    <section
      className={cn(
        "rounded-2xl border border-slate-700/70 bg-slate-950/55 shadow-[0_10px_40px_rgba(5,16,32,0.45)] backdrop-blur-sm",
        className,
      )}
      {...props}
    />
  );
}

export function PanelHeader({ className, ...props }: HTMLAttributes<HTMLDivElement>): JSX.Element {
  return <header className={cn("border-b border-slate-700/55 px-5 py-4", className)} {...props} />;
}

export function PanelBody({ className, ...props }: HTMLAttributes<HTMLDivElement>): JSX.Element {
  return <div className={cn("px-5 py-4", className)} {...props} />;
}

export function PanelTitle({ className, ...props }: HTMLAttributes<HTMLHeadingElement>): JSX.Element {
  return (
    <h3
      className={cn("text-xs font-semibold uppercase tracking-[0.18em] text-cyan-200/95", className)}
      {...props}
    />
  );
}
