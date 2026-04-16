import { ConsoleLayout } from "@/app/layout/console-layout";
import { AppProviders } from "@/app/providers/app-providers";
import { AppRouter } from "@/app/router/app-router";

export function App(): JSX.Element {
  return (
    <AppProviders>
      <ConsoleLayout>
        <AppRouter />
      </ConsoleLayout>
    </AppProviders>
  );
}
