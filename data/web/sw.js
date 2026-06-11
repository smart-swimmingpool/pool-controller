// Pool Controller — Service Worker v1
// Cache name includes date for easy version bumps
const CACHE = 'pool-ctrl-v1';
const STATIC_ASSETS = [
  '/',
  '/style.css',
  '/app.js',
  '/manifest.json',
  '/icon.svg'
];

// ── Install: pre-cache critical files ──
self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE)
      .then((cache) => cache.addAll(STATIC_ASSETS))
      .then(() => self.skipWaiting())
  );
});

// ── Activate: clean up old caches ──
self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(
        keys
          .filter((key) => key !== CACHE)
          .map((key) => caches.delete(key))
      )
    ).then(() => self.clients.claim())
  );
});

// ── Fetch: network-first with cache fallback ──
self.addEventListener('fetch', (event) => {
  // Only handle GET requests
  if (event.request.method !== 'GET') return;

  const url = new URL(event.request.url);

  // Skip API calls — always fetch from network (no stale data)
  if (url.pathname.startsWith('/api/')) {
    event.respondWith(fetch(event.request));
    return;
  }

  // Skip non-HTTP(S) schemes (e.g., chrome-extension://)
  if (!url.protocol.startsWith('http')) return;

  // Network-first strategy: try network, fall back to cache
  event.respondWith(
    fetch(event.request)
      .then((response) => {
        // Cache successful responses for future offline use
        if (response && response.status === 200) {
          const clone = response.clone();
          caches.open(CACHE).then((cache) => cache.put(event.request, clone));
        }
        return response;
      })
      .catch(() => {
        // Offline — serve from cache
        return caches.match(event.request).then((cached) => {
          if (cached) return cached;
          // If not in cache and offline, return a minimal offline page
          if (event.request.headers.get('Accept')?.includes('text/html')) {
            return new Response(
              '<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Pool Controller — Offline</title><style>body{background:#06121e;color:#8aadc4;font-family:system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;padding:2rem;text-align:center;line-height:1.6}h1{color:#00e5ff}p{color:#8aadc4}</style></head><body><h1>Pool Controller</h1><p>⚠️ Device is currently offline.<br>The dashboard will resume automatically when the connection is restored.</p></body></html>',
              { headers: { 'Content-Type': 'text/html; charset=utf-8' } }
            );
          }
          return new Response('Offline', { status: 503 });
        });
      })
  );
});
