import type { Config } from "tailwindcss";

const config: Config = {
  darkMode: ["class"],
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        border: "hsl(215 28% 18%)",
        input: "hsl(215 28% 18%)",
        ring: "hsl(188 95% 64%)",
        background: "hsl(220 30% 8%)",
        foreground: "hsl(210 40% 96%)",
        card: "hsl(222 31% 11%)",
        "card-foreground": "hsl(210 40% 96%)",
        primary: {
          DEFAULT: "hsl(188 95% 64%)",
          foreground: "hsl(220 50% 8%)",
        },
        muted: {
          DEFAULT: "hsl(223 20% 17%)",
          foreground: "hsl(216 20% 74%)",
        },
      },
      boxShadow: {
        panel: "0 12px 40px rgba(3, 18, 31, 0.55)",
      },
      backgroundImage: {
        grid: "radial-gradient(circle at 1px 1px, rgba(99, 179, 237, 0.16) 1px, transparent 0)",
      },
      keyframes: {
        "fade-up": {
          "0%": { opacity: "0", transform: "translateY(8px)" },
          "100%": { opacity: "1", transform: "translateY(0)" },
        },
      },
      animation: {
        "fade-up": "fade-up 400ms ease-out",
      },
    },
  },
  plugins: [],
};

export default config;
