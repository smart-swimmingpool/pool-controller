// Pool Controller — Service Worker v2
// Strategy: stale-while-revalidate for static assets.
//   - First visit: fetch from network, populate cache.
//   - Repeat visits: serve instantly from cache, refresh in background.
//   - API calls: always network (no stale data).
// Bump CACHE when deploying changed assets to force a clean slate.
const CACHE = 'pool-ctrl-v2';
const STATIC_ASSETS = [
  '/',
  '/style.css',
  '/app.js',
  '/manifest.json',
  '/icon.svg'
];

const OFFLINE_HTML = '<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Pool Controller — Offline</title><style>body{background:#06121e;color:#8aadc4;font-family:system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:100vh;padding:2rem;text-align:center;line-height:1.6}h1{color:#00e5ff}p{color:#8aadc4}</style></head><body><h1>Pool Controller</h1><p>⚠️ Device is currently offline.<br>The dashboard will resume automatically when the connection is restored.</p></body></html>';

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

// ── Fetch: stale-while-revalidate ──
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

  event.respondWith(
    caches.match(event.request).then((cached) => {
      // Navigation fallback: serve the cached app shell for dashboard routes
      // (/, /index.html) that are not cached under their exact URL. Other
      // server-rendered routes (e.g. /login) must not be replaced by the
      // dashboard shell — they fall through to the network fetch below.
      // Promise.resolve() normalizes the cache hit (a Response) and the
      // fallback lookup (a Promise) into one chainable promise.
      const isDashboardRoute = url.pathname === '/' || url.pathname === '/index.html';
      const cacheHit = cached || (
        isDashboardRoute && event.request.headers.get('Accept')?.includes('text/html')
          ? caches.match('/')
          : undefined
      );

      return Promise.resolve(cacheHit).then((hit) => {
        // Background refresh: fetch from network bypassing the HTTP cache
        // (so freshly uploaded assets are picked up), then update the cache.
        // The cache write is awaited so the promise below only settles once
        // the refresh is fully persisted.
        const networkFetch = fetch(event.request, { cache: 'no-store' })
          .then((response) => {
            if (response && response.status === 200) {
              const clone = response.clone();
              return caches.open(CACHE)
                .then((cache) => cache.put(event.request, clone))
                .then(() => response);
            }
            return response;
          })
          .catch(() => {
            // Offline — fall back to cache, then to a minimal offline page.
            if (hit) return hit;
            if (event.request.headers.get('Accept')?.includes('text/html')) {
              return new Response(OFFLINE_HTML, { headers: { 'Content-Type': 'text/html; charset=utf-8' } });
            }
            return new Response('Offline', { status: 503 });
          });

        // Keep the worker alive until the background refresh completes so
        // freshly uploaded assets are not left stale across reloads.
        event.waitUntil(networkFetch);

        // Serve the cached copy immediately when available.
        return hit || networkFetch;
      });
    })
  );
});
