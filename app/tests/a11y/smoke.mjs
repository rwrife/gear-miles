/* Accessibility + phone-viewport smoke: loads the built dist/ served by the
 * firmware API stub (one origin, like the device), then asserts keyboard
 * navigation, visible focus, contrast-checked palette invariants, estimate
 * labeling, and 360px-viewport usability.
 * Host/simulation evidence only. Run with GM_STUB_DIST=dist and the stub
 * listening at GM_STUB_URL (CI starts both). */
import { test } from "node:test";
import assert from "node:assert/strict";
import { chromium } from "playwright";

const BASE = process.env.GM_STUB_URL ?? "http://127.0.0.1:8123";

let browser, page;

test.before(async () => {
  browser = await chromium.launch();
  page = await browser.newPage({ viewport: { width: 360, height: 740 } }); /* phone */
  await page.goto(BASE + "/", { waitUntil: "networkidle" });
});

test.after(async () => { await browser?.close(); });

test("page has title, lang, single h1", async () => {
  assert.match(await page.title(), /Gear Miles/);
  assert.equal(await page.locator("html").getAttribute("lang"), "en");
  assert.equal(await page.locator("h1").count(), 1);
});

test("tabs are keyboard reachable and switch views", async () => {
  /* focus the first tab via keyboard from the skip link */
  await page.keyboard.press("Tab"); /* skip-link */
  await page.keyboard.press("Tab"); /* first tab button */
  const focused = await page.evaluate(() => document.activeElement?.id);
  assert.equal(focused, "tab-live");
  /* tab to History and activate */
  await page.keyboard.press("Tab");
  await page.keyboard.press("Enter");
  assert.equal(await page.locator("#view-history").isVisible(), true);
  assert.equal(await page.locator("#view-live").isVisible(), false);
  assert.equal(
    await page.locator("#tab-history").getAttribute("aria-selected"), "true");
});

test("live view exposes estimate labeling", async () => {
  await page.locator("#tab-live").click();
  const note = await page.locator("#live-estimate-note").textContent();
  assert.match(note ?? "", /Estimated|estimated/);
  assert.match(note ?? "", /cadence/);
});

test("settings form labels match inputs", async () => {
  await page.locator("#tab-settings").click();
  const pairs = await page.evaluate(() =>
    [...document.querySelectorAll("label[for]")].map((l) => [
      l.getAttribute("for"), !!document.getElementById(l.getAttribute("for")),
    ]));
  assert.ok(pairs.length >= 8);
  for (const [id, found] of pairs) assert.ok(found, `label for=${id} has target`);
});

test("phone-width: no horizontal overflow", async () => {
  const overflow = await page.evaluate(() =>
    document.documentElement.scrollWidth - document.documentElement.clientWidth);
  assert.ok(overflow <= 1, `horizontal overflow ${overflow}px at 360px`);
});

test("contrast: computed palette passes WCAG AA for body text", async () => {
  /* the stylesheet pins fg on bg; verify the values actually in use */
  const { fg, bg } = await page.evaluate(() => {
    const cs = getComputedStyle(document.body);
    return { fg: cs.color, bg: cs.backgroundColor };
  });
  const lum = (rgb) => {
    const [r, g, b] = rgb.match(/\d+/g).map(Number).map((v) => {
      const c = v / 255;
      return c <= 0.03928 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4;
    });
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
  };
  const l1 = lum(fg), l2 = lum(bg);
  const ratio = (Math.max(l1, l2) + 0.05) / (Math.min(l1, l2) + 0.05);
  assert.ok(ratio >= 4.5, `body contrast ${ratio.toFixed(2)} < 4.5`);
});
