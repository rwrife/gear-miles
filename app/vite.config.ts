import { defineConfig } from "vite";

/* base "./" -> relative asset URLs so the built SPA works from any origin
 * the device serves it from. Empty outDir hashing note: the firmware
 * integration step (documented in app/README.md) gzips dist/ and records a
 * sha256 manifest — embedding itself is a firmware issue, not this one. */
export default defineConfig({
  base: "./",
  build: {
    outDir: "dist",
    assetsDir: "assets",
    emptyOutDir: true,
  },
});
