# Release Metadata TLS Verification

**Status:** Draft · **Priority:** P1 · **Created:** 2026-06-04

---

## Background

`OtaUpdater::fetchLatestRelease()` uses `WiFiClientSecure::setInsecure()` to
fetch the GitHub API release metadata at
`https://api.github.com/repos/{owner}/{repo}/releases/latest`. The JSON response
supplies the `browser_download_url` that `startUpdate()` then passes to
`downloadAndApply()`.

Because the metadata fetch disables TLS certificate validation, a MITM on a
shared or untrusted network can:

1. Intercept the API response
2. Inject a fake release with a higher version tag
3. Point `downloadUrl_` to a malicious binary on any HTTPS host
4. The device will download and flash that binary via `downloadAndApply()`

This defeats the CA verification added to `downloadAndApply()`, since the
attacker controls which URL is downloaded.

## Requirements

### R1: TLS verification for metadata fetch

**Given** `OtaUpdater::fetchLatestRelease()` connecting to `api.github.com`
**When** the HTTPS connection is established
**Then** TLS certificate validation SHALL be enabled
**And** the SAME CA trust store SHALL be used as for the firmware binary
download

### R2: Response integrity validation

**Given** a fetched GitHub API release response
**When** the response is parsed
**Then** the `tag_name` SHALL match a valid semver pattern (`v?MAJOR.MINOR.PATCH`)
**And** the `browser_download_url` SHALL match the expected GitHub CDN host
pattern (`*.github.com`, `objects.githubusercontent.com`, etc.)
**And** the release SHALL be from the configured repository

### R3: Replay protection

**Given** a previously fetched (and potentially cached) release metadata
**When** the metadata is used for an OTA update
**Then** a fresh fetch SHALL be performed — never reuse cached metadata from a
previous check

## Design

### Approach

1. **TLS for metadata:** Replace `setInsecure()` with the same CA bundle
   approach used for downloads (see `github-ca-chain` spec).
2. **URL validation:** Add URL hostname whitelist for `browser_download_url`.
3. **Semver validation:** Validate `tag_name` against semver regex before
   accepting as `latestVersion_`.
4. **Fresh fetch guard:** Verify that `lastCheckTime_` is recent (within the
   last loop cycle) before using cached metadata for an update.

### Affected files

| File                 | Change                                                           |
| -------------------- | ---------------------------------------------------------------- |
| `src/OtaUpdater.cpp` | Replace `setInsecure()` with CA bundle in `fetchLatestRelease()` |
| `src/OtaUpdater.cpp` | Add URL hostname validation after parsing                        |
| `src/OtaUpdater.cpp` | Add semver validation for `tag_name`                             |
| `src/OtaUpdater.hpp` | Add validation constants and helper declarations                 |

### API changes

```cpp
// New constants in OtaUpdater.hpp
static constexpr size_t kMaxDownloadUrlLen = 256;

// New private helpers
static bool isValidReleaseUrl(const String &url);
static bool isValidSemverTag(const String &tag);
static const char* kAllowedDownloadHosts[];
```

## Tasks

- [ ] **T1:** Replace `setInsecure()` with CA bundle TLS in
      `fetchLatestRelease()` (shared with `github-ca-chain` T4)
- [ ] **T2:** Implement `isValidReleaseUrl()` — whitelist check against
      `kAllowedDownloadHosts`
- [ ] **T3:** Implement `isValidSemverTag()` — reject non-semver tag names
- [ ] **T4:** Add hostname validation for `browser_download_url` after JSON
      parsing
- [ ] **T5:** Add semver tag validation for `tag_name` after JSON parsing
- [ ] **T6:** Ensure fresh fetch before `startUpdate()` — reject if
      `lastCheckTime_` is older than 1 minute relative to update trigger
- [ ] **T7:** Write unit tests for URL and semver validation

## Test Requirements

- **T1:** Metadata fetch succeeds with TLS verification against live
  `api.github.com`
- **T2–T4:** Unit test: valid hostnames pass, invalid hostnames are rejected
- **T3–T5:** Unit test: `v1.2.3` passes, `v1.2` / `latest` / `evil-script`
  are rejected
- **T6:** Integration test: stale metadata triggers re-fetch before update
- **T7:** Edge cases: missing `tag_name`, missing `browser_download_url`,
  malformed JSON, network timeout during metadata fetch
