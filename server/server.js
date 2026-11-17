'use strict';

const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { Transform } = require('node:stream');
const { pipeline } = require('node:stream/promises');

const host = process.env.HOST || '0.0.0.0';
const port = Number(process.env.PORT || 25832);
const uploadToken = process.env.TEAM_UPLOAD_TOKEN || '';
const maxUploadBytes = Number(process.env.MAX_UPLOAD_BYTES || 1024 * 1024 * 1024);
const dataDir = path.resolve(process.env.PACK_DIR || path.join(__dirname, 'packs'));
const newsDir = path.resolve(process.env.NEWS_DIR || path.join(__dirname, 'news'));
const zipPath = path.join(dataDir, 'current.zip');
const metadataPath = path.join(dataDir, 'current.json');
const adminPath = path.join(__dirname, 'admin.html');
const hashCache = new Map();
let uploadBusy = false;
const maxNewsFileBytes = Number(process.env.MAX_NEWS_FILE_BYTES || 512 * 1024);
const maxNewsEntries = Math.max(1, Number(process.env.MAX_NEWS_ENTRIES || 100));

function send(res, status, body, type = 'application/json; charset=utf-8') {
  res.writeHead(status, { 'content-type': type, 'cache-control': 'no-store' });
  res.end(Buffer.isBuffer(body) ? body : (typeof body === 'string' ? body : JSON.stringify(body)));
}

function safeHeader(value, fallback, maxLength = 80) {
  const text = String(value || fallback).replace(/[\r\n\0]/g, '').trim();
  return text.slice(0, maxLength) || fallback;
}

async function readMetadata() {
  try {
    return JSON.parse(await fs.promises.readFile(metadataPath, 'utf8'));
  } catch (error) {
    if (error.code === 'ENOENT') return null;
    console.warn('Ignoring invalid current.json metadata:', error.message);
    return null;
  }
}

async function hasZipHeader(filePath) {
  const handle = await fs.promises.open(filePath, 'r');
  try {
    const prefix = Buffer.alloc(4);
    const { bytesRead } = await handle.read(prefix, 0, prefix.length, 0);
    return bytesRead >= 2 && prefix[0] === 0x50 && prefix[1] === 0x4b;
  } finally {
    await handle.close();
  }
}

function hashFile(filePath) {
  return new Promise((resolve, reject) => {
    const hash = crypto.createHash('sha256');
    const input = fs.createReadStream(filePath);
    input.on('data', chunk => hash.update(chunk));
    input.on('error', reject);
    input.on('end', () => resolve(hash.digest('hex')));
  });
}

async function findLatestPack() {
  await fs.promises.mkdir(dataDir, { recursive: true });
  const entries = await fs.promises.readdir(dataDir, { withFileTypes: true });
  const candidates = [];
  for (const entry of entries) {
    if (!entry.isFile() || !entry.name.toLowerCase().endsWith('.zip')) continue;
    const filePath = path.join(dataDir, entry.name);
    try {
      const stat = await fs.promises.stat(filePath);
      if (stat.size < 22 || stat.size > maxUploadBytes || !(await hasZipHeader(filePath))) continue;
      candidates.push({ fileName: entry.name, filePath, stat });
    } catch (error) {
      if (error.code !== 'ENOENT') console.warn(`Skipping ${entry.name}:`, error.message);
    }
  }
  candidates.sort((a, b) => b.stat.mtimeMs - a.stat.mtimeMs || a.fileName.localeCompare(b.fileName));
  const selected = candidates[0];
  if (!selected) return null;

  const cacheKey = `${selected.fileName}:${selected.stat.size}:${selected.stat.mtimeMs}`;
  let sha256 = hashCache.get(cacheKey);
  if (!sha256) {
    sha256 = await hashFile(selected.filePath);
    if (hashCache.size > 32) hashCache.clear();
    hashCache.set(cacheKey, sha256);
  }

  const saved = selected.fileName.toLowerCase() === 'current.zip' ? await readMetadata() : null;
  const savedMatches = saved && saved.size === selected.stat.size && saved.sha256 === sha256;
  const fallbackName = path.parse(selected.fileName).name;
  return {
    id: 'current',
    name: safeHeader(savedMatches ? saved.name : fallbackName, 'Team Instance'),
    version: safeHeader(savedMatches ? saved.version : selected.stat.mtime.toISOString().slice(0, 10), 'latest', 40),
    updatedAt: selected.stat.mtime.toISOString(),
    size: selected.stat.size,
    sha256,
    downloadUrl: '/api/packs/current.zip',
    fileName: selected.fileName,
    filePath: selected.filePath,
  };
}

async function readNews() {
  await fs.promises.mkdir(newsDir, { recursive: true });
  const files = await fs.promises.readdir(newsDir, { withFileTypes: true });
  const candidates = [];
  for (const entry of files) {
    if (!entry.isFile() || path.extname(entry.name).toLowerCase() !== '.md') continue;
    const filePath = path.join(newsDir, entry.name);
    try {
      const stat = await fs.promises.stat(filePath);
      if (stat.size > maxNewsFileBytes) {
        console.warn(`Skipping news file ${entry.name}: exceeds ${maxNewsFileBytes} bytes.`);
        continue;
      }
      candidates.push({ entry, filePath, stat });
    } catch (error) {
      if (error.code !== 'ENOENT') console.warn(`Skipping news file ${entry.name}:`, error.message);
    }
  }
  candidates.sort((a, b) => b.stat.mtimeMs - a.stat.mtimeMs || a.entry.name.localeCompare(b.entry.name));

  const news = [];
  for (const { entry, filePath, stat } of candidates.slice(0, maxNewsEntries)) {
    try {
      let content = await fs.promises.readFile(filePath, 'utf8');
      content = content.replace(/^\uFEFF/, '');
      const heading = content.match(/^\s*#\s+(.+?)\s*#*\s*$/m);
      news.push({
        id: entry.name,
        title: safeHeader(heading ? heading[1] : path.parse(entry.name).name, 'Team news', 160),
        updatedAt: stat.mtime.toISOString(),
        content,
      });
    } catch (error) {
      if (error.code !== 'ENOENT') console.warn(`Could not read news file ${entry.name}:`, error.message);
    }
  }
  return { news };
}

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
  try {
    if (req.method === 'GET' && (url.pathname === '/' || url.pathname === '/admin')) {
      const html = await fs.promises.readFile(adminPath);
      return send(res, 200, html, 'text/html; charset=utf-8');
    }

    if (req.method === 'GET' && url.pathname === '/api/packs') {
      const pack = await findLatestPack();
      if (!pack) return send(res, 200, { packs: [] });
      const { fileName, filePath, ...publicPack } = pack;
      return send(res, 200, { packs: [publicPack] });
    }

    if (req.method === 'GET' && url.pathname === '/api/packs/current.zip') {
      const pack = await findLatestPack();
      if (!pack) return send(res, 404, { error: 'No instance ZIP has been uploaded yet.' });
      res.writeHead(200, {
        'content-type': 'application/zip',
        'content-length': pack.size,
        'content-disposition': 'attachment; filename="team-instance.zip"',
        'cache-control': 'no-store',
      });
      return fs.createReadStream(pack.filePath).pipe(res);
    }

    if (req.method === 'GET' && url.pathname === '/api/news') {
      return send(res, 200, await readNews());
    }

    if (req.method === 'POST' && url.pathname === '/api/packs') {
      if (!uploadToken) return send(res, 503, { error: 'Set TEAM_UPLOAD_TOKEN before enabling uploads.' });
      if (req.headers.authorization !== `Bearer ${uploadToken}`) return send(res, 401, { error: 'Upload token is incorrect.' });
      if (uploadBusy) return send(res, 409, { error: 'Another upload is in progress.' });
      if (req.headers['content-type'] && !req.headers['content-type'].startsWith('application/zip') && req.headers['content-type'] !== 'application/octet-stream') {
        return send(res, 415, { error: 'Upload the ZIP file as application/zip.' });
      }
      uploadBusy = true;
      await fs.promises.mkdir(dataDir, { recursive: true });
      const tempPath = `${zipPath}.${process.pid}.part`;
      const hash = crypto.createHash('sha256');
      let size = 0;
      let prefix = Buffer.alloc(0);
      let checkedZipHeader = false;
      const inspect = new Transform({
        transform(chunk, _encoding, callback) {
          size += chunk.length;
          if (size > maxUploadBytes) return callback(new Error(`ZIP exceeds the ${maxUploadBytes} byte upload limit.`));
          hash.update(chunk);
          if (!checkedZipHeader) {
            prefix = Buffer.concat([prefix, chunk]).subarray(0, 4);
            if (prefix.length >= 4) {
              checkedZipHeader = true;
              if (prefix[0] !== 0x50 || prefix[1] !== 0x4b) return callback(new Error('The uploaded file is not a ZIP archive.'));
            }
          }
          callback(null, chunk);
        },
        flush(callback) {
          if (!checkedZipHeader || size < 22) return callback(new Error('The ZIP file is empty or incomplete.'));
          callback();
        },
      });
      try {
        await pipeline(req, inspect, fs.createWriteStream(tempPath, { flags: 'w' }));
        await fs.promises.rename(tempPath, zipPath);
        const stat = await fs.promises.stat(zipPath);
        const sha256 = hash.digest('hex');
        const metadata = {
          id: 'current',
          name: safeHeader(req.headers['x-pack-name'], 'Team Instance'),
          version: safeHeader(req.headers['x-pack-version'], 'latest', 40),
          updatedAt: stat.mtime.toISOString(),
          size,
          sha256,
          downloadUrl: '/api/packs/current.zip',
        };
        const metaTempPath = `${metadataPath}.${process.pid}.part`;
        await fs.promises.writeFile(metaTempPath, JSON.stringify(metadata, null, 2), 'utf8');
        await fs.promises.rename(metaTempPath, metadataPath);
        hashCache.clear();
        return send(res, 201, metadata);
      } catch (error) {
        await fs.promises.rm(tempPath, { force: true });
        const status = error.message.includes('limit') ? 413 : 400;
        return send(res, status, { error: error.message || 'Upload failed.' });
      } finally {
        uploadBusy = false;
      }
    }

    return send(res, 404, { error: 'Not found.' });
  } catch (error) {
    console.error(error);
    if (!res.headersSent) send(res, 500, { error: 'The server could not complete the request.' });
    else res.destroy(error);
  }
});

server.listen(port, host, () => {
  console.log(`Prism team instance host listening on http://${host}:${port}`);
  if (!uploadToken) console.warn('Web uploads are disabled until TEAM_UPLOAD_TOKEN is set. ZIP files placed directly in packs/ are still scanned.');
  console.log(`ZIP storage: ${dataDir}`);
});
