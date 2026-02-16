---
applyTo: ".github/workflows/**"
description: "CI/CD workflow instructions"
---

- CI workflows must run lint, build, tests, and memory analysis steps in sequence.
- Ensure caching of PlatformIO artifacts to speed up CI.
- Cancel redundant runs on pull request update events.
- Use environment variables for sensitive data, do not hardcode secrets.
- CI must fail on any linting, build, or test errors; do not allow warnings to pass.
- Include a step to upload test results and coverage reports for analysis.
