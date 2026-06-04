# OTA Binary Verification

**Status:** Draft · **Priority:** P1 · **Created:** 2026-06-04

---

## Background

The OTA download path (`OtaUpdater::downloadAndApply()`) streams the firmware
binary directly into `Update.write()` without any integrity check before
committing. A MITM on the shared network can replace the `.bin` during the TLS
download (or via the insecure metadata) and the device will flash the
attacker-controlled binary.

The download uses `WiFiClientSecure` with `setCACert(kGitHubRootCA)`, so TLS
termination is verified — but the stream bytes are not hashed or signed before
`Update.end(true)` finalises the flash.

## Requirements

### R1: SHA-256 hash verification after download

**Given** a downloaded firmware binary in the OTA buffer
**When** `Update.end()` is about to be called
**Then** the hash of the received stream SHALL be computed and compared against
the expected SHA-256 published in the GitHub release metadata
**And** if the hash does not match, `Update.end(false)` SHALL be called and the
update SHALL be aborted

### R2: Ed25519 signature verification (long-term)

**Given** a firmware binary signed with the project's Ed25519 private key
**When** the binary is downloaded
**Then** the signature SHALL be verified using the compiled-in public key before
`Update.end(true)`
**And** the signature asset (`.bin.sig`) SHALL be published alongside each
firmware binary in the GitHub release

### R3: Graceful failure reporting

**Given** a verification failure (hash mismatch or bad signature)
**When** the update is aborted
**Then** `updateAvailable_` SHALL remain `true`
**And** `statusMessage_` SHALL describe the failure reason
**And** the existing firmware SHALL continue running unaffected

## Design

### Approach (incremental)

1. **Phase 1 — SHA-256 hashing:** Compute SHA-256 of stream in
   `downloadAndApply()` after download completes but before `Update.end()`.
   Compare against expected hash obtained from GitHub release metadata
   (`assets` array).
2. **Phase 2 — Ed25519 signatures:** Add a release-signing script. Verify
   signature with compiled-in public key.

### Affected files

| File | Change |
|------|--------|
| `src/OtaUpdater.cpp` | Add hash computation and verification in `downloadAndApply()` |
| `src/OtaUpdater.hpp` | Add hash digest buffer; expose `expectedHash` |
| `scripts/sign-release.sh` | New: Ed25519 signing script for GitHub Actions |
| `.github/workflows/release.yml` | Add signing step to release pipeline |
| `src/OtaUpdater.hpp` | Add public key constant |

### API changes

```cpp
// New constants
static constexpr size_t kSha256DigestSize = 32;
static const String kSigningPublicKey;  // hex-encoded Ed25519 public key

// Modified
static bool downloadAndApply(const String &url, const String &expectedSha256);
```

## Tasks

- [ ] **T1:** Add SHA-256 computation in `downloadAndApply()` using
      `mbedtls_sha256_ret()` (ESP32 mbedTLS built-in)
- [ ] **T2:** Extend `fetchLatestRelease()` to extract `sha256` from release
      assets metadata; fall back gracefully if absent
- [ ] **T3:** Implement hash comparison and abort path
- [ ] **T4:** Create `scripts/sign-release.sh` for Ed25519 signing
- [ ] **T5:** Add signing step to CI release workflow
- [ ] **T6:** Add verification phase 2 (Ed25519) behind feature flag
- [ ] **T7:** Write unit tests for hash/signature verification logic

## Test Requirements

- **T1–T3:** Integration test: mock download stream with known SHA-256;
  verify correct hash passes, incorrect hash aborts
- **T4–T5:** CI pipeline produces signed artifacts; verify signature offline
- **T6:** Unit test: known-good signature passes; tampered binary fails
- **T7:** Edge cases: empty stream, truncated stream, hash mismatch,
  missing hash metadata
