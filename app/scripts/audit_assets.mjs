/* Bundle audit: the built dist/ must contain no external network hosts —
 * no CDN, no analytics, no external fonts (docs/protocol.md + issue #7).
 * Allowed: same-origin relative URLs, data: URIs, and localhost literals
 * that only appear in comments/maps (we fail those too, to be strict). */
import { readdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";

const dist = new URL("../dist", import.meta.url).pathname;
const urlRe = /\bhttps?:\/\/[^\s"'`)\\]+/g;
const protoRe = /\b\/\/(?!127\.0\.0\.1|localhost)[a-z0-9-]+(\.[a-z0-9-]+)+/gi;

const offenders = [];
function walk(dir) {
  for (const name of readdirSync(dir)) {
    const p = join(dir, name);
    if (statSync(p).isDirectory()) { walk(p); continue; }
    const text = readFileSync(p, "utf8");
    for (const m of text.matchAll(urlRe)) offenders.push([p, m[0]]);
    for (const m of text.matchAll(protoRe)) offenders.push([p, m[0]]);
  }
}
walk(dist);
if (offenders.length) {
  console.error("ASSET AUDIT FAIL — external URLs in built assets:");
  for (const [f, u] of offenders) console.error(`  ${f}: ${u}`);
  process.exit(1);
}
console.log("ASSET AUDIT PASS — no external hosts in dist/");
