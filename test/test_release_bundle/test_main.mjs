import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { existsSync, readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const docs = resolve(here, "../../docs");
const manifest = JSON.parse(readFileSync(join(docs, "manifest.json"), "utf8"));
assert.equal(manifest.name, "Gas Monitor");
assert.equal(manifest.version, "0.1.0");
assert.equal(manifest.builds[0].chipFamily, "ESP32-S3");
assert.deepEqual(manifest.builds[0].parts, [
  { path: "firmware/bootloader.bin", offset: 0 },
  { path: "firmware/partition-table.bin", offset: 32768 },
  { path: "firmware/gas_monitor.bin", offset: 65536 },
]);

const checksums = JSON.parse(readFileSync(join(docs, "checksums.json"), "utf8"));
for (const part of manifest.builds[0].parts) {
  const path = join(docs, part.path);
  assert.ok(existsSync(path), `${part.path} must exist`);
  const hash = createHash("sha256").update(readFileSync(path)).digest("hex");
  assert.equal(checksums[part.path], hash, `${part.path} checksum mismatch`);
}

const installer = readFileSync(join(docs, "index.html"), "utf8");
assert.match(installer, /esp-web-install-button/);
assert.match(installer, /manifest="manifest\.json"/);
assert.match(installer, /Google Chrome|Microsoft Edge/);
assert.match(installer, /GPIO5/);
const wiring = readFileSync(join(docs, "wiring.md"), "utf8");
for (const text of [installer, wiring]) {
  for (const gpio of [9, 10, 11, 12]) assert.match(text, new RegExp(`GPIO${gpio}\\b`));
  assert.doesNotMatch(text, /GPIO15\b|GPIO16\b/);
}
console.log("Gas Monitor release bundle contract passed");
