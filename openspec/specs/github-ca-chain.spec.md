# GitHub CA Chain for OTA Downloads

**Status:** Draft · **Priority:** P1 · **Created:** 2026-06-04

---

## Background

`OtaUpdater::downloadAndApply()` uses `WiFiClientSecure::setCACert()` with the
ISRG Root X1 certificate (Let's Encrypt) pinned in PROGMEM. GitHub release
download URLs are served via GitHub's CDN, which may use a Sectigo or other CA
chain rather than Let's Encrypt. When the pinned root CA does not match the
server's chain, `http.GET()` fails TLS before any bytes are downloaded, and
every OTA update attempt is blocked.

Additionally, `fetchLatestRelease()` uses `setInsecure()` (no cert validation),
which defeats TLS entirely for the metadata fetch.

## Requirements

### R1: Reliable TLS for firmware downloads

**Given** an ESP32 device initiating an OTA download from GitHub releases
**When** `WiFiClientSecure` connects to the GitHub CDN download host
**Then** TLS certificate validation SHALL succeed using a CA bundle that
includes the actual certificate chains used by GitHub's CDN
**And** the device SHALL NOT rely on a single pinned root CA

### R2: Single CA bundle for both metadata and download

**Given** both `fetchLatestRelease()` and `downloadAndApply()` establish TLS
connections to GitHub
**When** the CA trust store is configured
**Then** the SAME CA bundle SHALL be used for both the API metadata fetch and
the binary download

### R3: Graceful TLS fallback on flash-constrained devices

**Given** an ESP8266 or other device with insufficient flash for a full CA
bundle
**When** TLS verification is not possible
**Then** a compile-time flag SHALL allow falling back to `setInsecure()` with a
clear log warning

## Design

### Approach

1. **ESP32 (recommended):** Use `WiFiClientSecure::setCACertBundle()` with
   ESP32's built-in `x509_crt_bundle` which ships ~130 root CAs including all
   major chains used by GitHub CDN.
2. **Fallback:** If bundle is unavailable, use a curated subset CA bundle as a
   PROGMEM string covering Let's Encrypt, Sectigo, and DigiCert roots.
3. **Metadata fetch:** Replace `setInsecure()` in `fetchLatestRelease()` with
   the same CA trust store used for downloads.

### Affected files

| File | Change |
|------|--------|
| `src/OtaUpdater.cpp` | Replace `setCACert(kGitHubRootCA)` with `setCACertBundle()` |
| `src/OtaUpdater.cpp` | Replace `setInsecure()` in `fetchLatestRelease()` |
| `src/OtaUpdater.hpp` | Add compile-time flag `OTA_USE_CA_BUNDLE` |
| `platformio.ini` | Add `-DCORE_SSL_CERT_BUNDLE` build flag for ESP32 |
| `src/CACertBundle.hpp` | New: fallback bundle with curated root CAs |

### API changes

```cpp
// No public API changes. Internal behavior change:
// downloadAndApply() uses setCACertBundle() instead of setCACert(kGitHubRootCA)
// fetchLatestRelease() uses setCACertBundle() instead of setInsecure()
```

## Tasks

- [ ] **T1:** Enable `CONFIG_ESP_TLS_USING_MBEDTLS_CERT_BUNDLE` in sdkconfig
      for ESP32 builds
- [ ] **T2:** Add `-DCORE_SSL_CERT_BUNDLE` build flag in `platformio.ini`
- [ ] **T3:** Replace `setCACert(kGitHubRootCA)` with `setCACertBundle()` in
      `downloadAndApply()`
- [ ] **T4:** Replace `setInsecure()` with `setCACertBundle()` (or equivalent
      CA trust) in `fetchLatestRelease()`
- [ ] **T5:** Create `CACertBundle.hpp` with curated fallback bundle for
      ESP8266 / flash-constrained builds
- [ ] **T6:** Add compile-time fallback to `setInsecure()` with log warning
      when CA bundle is not available
- [ ] **T7:** Test OTA download against live GitHub releases with the new
      CA bundle

## Test Requirements

- **T3:** OTA download succeeds on ESP32 with `setCACertBundle()`
- **T4:** `fetchLatestRelease()` succeeds with CA verification enabled
- **T5–T6:** On ESP8266 or when flag is unset, compilation succeeds and OTA
  falls back with warning
- **T7:** Integration test: verify TLS handshake with actual GitHub CDN
  endpoints (api.github.com, objects.githubusercontent.com)
