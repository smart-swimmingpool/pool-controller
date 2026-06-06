// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// FW_VERSION is provided as a build flag (-D FW_VERSION="x.y.z") via platformio.ini.
// This file only provides a fallback for compilation without the build flag.
// FW_VERSION is auto-maintained by release-please — do not edit manually.

#ifndef VERSION_H
#define VERSION_H

#ifndef FW_VERSION
#define FW_VERSION "0.0.0"  // x-release-please-version
#endif

#endif  // VERSION_H
