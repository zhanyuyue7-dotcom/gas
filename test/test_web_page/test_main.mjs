import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const html = readFileSync(resolve(here, "../../main/web_page.html"), "utf8");

assert.match(html, /--background:#faf9f5/);
assert.match(html, /id="hcho"/);
assert.match(html, /id="tvoc"/);
assert.match(html, /id="eco2"/);
assert.match(html, /eco2Estimated|估算/);
assert.match(html, /GB\/T 18883-2022/);
assert.match(html, /if\(polling\)return/);
assert.match(html, /fetch\('\/api\/status'/);
assert.match(html, /@media\(max-width:680px\)/);
assert.doesNotMatch(html, /localStorage/);
console.log("Dashboard source contract passed");
