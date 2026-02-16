---
applyTo: "test/**"
description: "Testing-specific guidelines for this repo"
---

- Always write tests that cover edge cases.
- Tests must follow the organization and naming patterns in `AGENTS.md`.
- Use mock objects or simulations to isolate hardware interactions.
- Do not introduce CI failures for intermittent timing issues.
- Ensure tests are deterministic and can run in any order.
- Document test cases clearly, especially for complex scenarios.
