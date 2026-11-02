// Exercise the exact publishing script embedded in build.yml without network access.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import crypto from 'node:crypto';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const workflow = fs.readFileSync(path.join(root, '.github/workflows/build.yml'), 'utf8');
const marker = '          script: |\n';
assert(workflow.includes(marker), 'Publishing script must exist');
const code = workflow.slice(workflow.indexOf(marker) + marker.length)
  .split('\n').map(line => line.startsWith('            ') ? line.slice(12) : line).join('\n');
const AsyncFunction = Object.getPrototypeOf(async function () {}).constructor;
const publish = new AsyncFunction('require', 'context', 'github', 'core', code);
const sha = data => crypto.createHash('sha256').update(data).digest('hex');

async function scenario({tag = 'v0.1.0', badChecksum = false, existingMismatch = false} = {}) {
  const nro = Buffer.from('verified fixture NRO');
  const zip = Buffer.from('verified fixture ZIP');
  const files = {
    'dist/NinMagicFoil.nro': nro,
    'dist/ninmagicfoil.zip': zip,
    'dist/verification.json': Buffer.from(JSON.stringify({version: '0.1.0', nacp_name: 'NinMagicFoil',
      own_update_channel_checked: true, sha256: sha(nro)})),
    'dist/SHA256SUMS': Buffer.from(`${badChecksum ? '0'.repeat(64) : sha(nro)}  NinMagicFoil.nro\n${sha(zip)}  ninmagicfoil.zip\n`),
  };
  const events = [];
  const repository = {
    getReleaseByTag: async () => {throw Object.assign(new Error('Not found'), {status: 404});},
    createRelease: async input => {events.push('create-draft'); assert.equal(input.draft, true); return {data: {id: 1, draft: true}};},
    listReleaseAssets: async () => {},
    uploadReleaseAsset: async input => {events.push(`upload:${input.name}`); return {data: {id: 2, size: input.data.length, digest: `sha256:${sha(input.data)}`}};},
    updateRelease: async input => {events.push('publish'); assert.equal(input.draft, false); return {data: {html_url: 'https://github.com/ANE-JV/ninmagicfoil/releases/tag/v0.1.0'}};},
  };
  const github = {rest: {repos: repository}, paginate: async () => existingMismatch
    ? [{id: 3, name: 'ninmagicfoil.zip', size: zip.length, digest: `sha256:${'0'.repeat(64)}`}]
    : []};
  const requireMock = name => ({fs: {readFileSync: (name, encoding) => {
    name = name.replaceAll('\\', '/');
    assert(Object.hasOwn(files, name)); return encoding ? files[name].toString(encoding) : files[name];
  }}, crypto, path})[name];
  let error;
  try { await publish(requireMock, {ref: `refs/tags/${tag}`, repo: {owner: 'ANE-JV', repo: 'ninmagicfoil'}}, github, {notice: () => {}}); }
  catch (caught) {error = caught;}
  return {events, error};
}

const good = await scenario();
assert.equal(good.error, undefined);
assert.deepEqual(good.events, ['create-draft', 'upload:ninmagicfoil.zip', 'upload:NinMagicFoil.nro',
  'upload:SHA256SUMS', 'upload:verification.json', 'publish']);
for (const bad of [{tag: 'v0.2.0'}, {badChecksum: true}]) {
  const result = await scenario(bad);
  assert(result.error);
  assert.deepEqual(result.events, []);
}
const mismatch = await scenario({existingMismatch: true});
assert(mismatch.error.message.includes('digest differs'));
assert.deepEqual(mismatch.events, ['create-draft']);
console.log('Release workflow: matching version/hash only, draft before upload, no overwrite on mismatch: passed');
