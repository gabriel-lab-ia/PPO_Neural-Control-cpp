import type { ReactNode } from "react";

interface ConsoleLayoutProps {
  children: ReactNode;
}

export function ConsoleLayout({ children }: ConsoleLayoutProps): JSX.Element {
  return <>{children}</>;
}
