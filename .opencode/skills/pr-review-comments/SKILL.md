---
name: pr-review-comments
description: "Process PR review comments (GitHub Codex or human) — fetch unresolved threads, triage by priority, fix per-comment with individual commits, reply with fix summary, resolve threads, and escalate larger items via OpenSpec + GitHub issue. Use when review comments arrive on an open PR, when triaging Codex review output, or when a PR needs systematic comment processing. 🇩🇪 Deutsche Trigger: Review-Kommentare, Codex Review, PR Review, Thread auflösen, Kommentar beantworten, Code Review, OpenSpec, GitHub Issue."
keywords:
  - pr review
  - review comments
  - code review
  - codex review
  - pr feedback
  - review thread
  - resolve thread
  - github review
  - openspec
  - github issue
  - review triage
  - pr comments
---

# PR Review Comments — Pool Controller

Systematic workflow for processing GitHub PR review comments (Codex, human, or
automated). Each comment gets either a **fix commit** (→ reply → resolve) or a
**deferral** (→ OpenSpec spec → GitHub issue → reply with link → leave unresolved).

## Prerequisites

- `gh` CLI authenticated (`gh auth status`)
- Branch checked out locally
- Working tree clean before starting

## Workflow Overview

```
Fetch PR review comments
  │
  ├── For EACH unresolved comment ──────────────────────────┐
  │                                                          │
  │  Triage: Fixable or Conceptual?                          │
  │  │                                                       │
  │  ├─ Fixable (P2/P3, concrete bug or code change)         │
  │  │  ├─ Determine fix scope (file + lines)                │
  │  │  ├─ Apply fix (single, focused change)                │
  │  │  ├─ Build + test (make lint, platformio build)        │
  │  │  ├─ Commit with conventional commit message           │
  │  │  ├─ git push                                          │
  │  │  ├─ Reply to thread with:                             │
  │  │  │  "✅ Fixed in `<commit-hash>`: <one-line summary>" │
  │  │  └─ Mark thread as resolved                           │
  │  │                                                       │
  │  └─ Conceptual (P1, new feature, architectural)          │
  │     ├─ Create OpenSpec spec document                     │
  │     ├─ Create OpenSpec change marker                     │
  │     ├─ Create GitHub issue (P1, enhancement/security)    │
  │     ├─ Commit spec + marker separately                   │
  │     ├─ git push                                          │
  │     └─ Reply to thread with:                             │
  │        "📋 Tracked in #<issue>: <summary>"               │
  │        (leave thread unresolved — addressed via issue)   │
  │                                                          │
  └─ After all comments processed ──→ Request re-review      │
     (re-request review from Codex or human via PR page)     │
```

## Step-by-Step

### 1. Fetch PR Review Comments

```bash
# PR with number N (from current branch or explicit)
gh pr view N --json title,body,comments,reviews

# List all review comments with their thread IDs
gh api "repos/:owner/:repo/pulls/N/comments" --paginate \
  --jq '.[] | select(.pull_request_review_id != null) |
    {id, path, line, body, thread_id: .thread_id, author: .user.login}'

# Get all unresolved threads (GraphQL)
gh api graphql -f query='
  query($owner:String!,$repo:String!,$pr:Int!) {
    repository(owner:$owner,name:$repo) {
      pullRequest(number:$pr) {
        reviewThreads(first:100) {
          nodes {
            id
            isResolved
            isOutdated
            path
            line
            comments(first:10) {
              nodes { body }
}}}}}}' -f owner="$OWNER" -f repo="$REPO" -F pr="$PR"
```

### 2. Triage Each Comment

For each unresolved, non-outdated thread, classify:

| Criterion | Fixable (fix now)      | Conceptual (defer)                  |
| --------- | ---------------------- | ----------------------------------- |
| Scope     | Small, bounded change  | Architectural / multi-file          |
| Priority  | P2 or P3               | P1                                  |
| Certainty | Clear what to change   | Needs design discussion             |
| Type      | Bug, lint, typo, style | New feature, security, refactor     |
| Example   | "Missing null check"   | "Add binary signature verification" |

### 3. Fix & Commit (per comment)

**One commit per review comment.** Never batch multiple comments into one
commit — this makes review history and cherry-picking cleaner.

```bash
# 1. Apply fix (minimal, focused on one thing)
# 2. Build + lint
make lint-fix
make lint
platformio build

# 3. Commit (conventional commit format)
git add -A
git commit -m "fix(scope): address review: <what the review pointed out>

Addresses PR #N review comment.

Reviewed-by: Codex"

# 4. Push
git push
```

Use the project's [conventional-commits](skill:conventional-commits) format.
Common types for review fixes:

| Type              | When                         |
| ----------------- | ---------------------------- |
| `fix(scope)`      | Bug fix or correctness issue |
| `refactor(scope)` | Code structure improvement   |
| `style(scope)`    | Linting, formatting, naming  |
| `test(scope)`     | Missing or broken tests      |
| `docs(scope)`     | Comment or docstring fixes   |
| `chore(scope)`    | Build, config, CI            |
| `security(scope)` | Security vulnerability       |

Choose the scope (`web`, `mqtt`, `wifi`, `config`, `ota`, `sensor`, `relay`,
`system`, `ha`, `ntp`, `ci`, `deps`, `docs` — see conventional-commits skill).

### 4. Reply to Thread

Get the thread ID from the review comment:

```bash
# Find thread ID for a specific comment
gh api "repos/:owner/:repo/pulls/N/comments" \
  --jq '.[] | select(.path == "src/WebPortal.cpp" and .line == 42) | .thread_id'
```

Reply to the thread (adds a comment to the review thread):

```bash
gh api graphql -f query='
  mutation($body:String!,$threadId:ID!) {
    addPullRequestReviewThreadReply(input:{
      body:$body, pullRequestId:$prId, threadId:$threadId
    }) { comment { url } }
  }' -f body="✅ Fixed in \`$(git rev-parse --short HEAD)\`: <one-line summary>" \
  -f threadId="PRRT_kwDO..."
```

**Reply format for fixes:**

```
✅ Fixed in `<short-sha>`: <one-line summary>

<optional detail / link to code>
```

**Reply format for deferred items:**

```
📋 Tracked in #<issue-number>: <title>

OpenSpec: `openspec/specs/<spec-name>.spec.md`
Priority: P1

<optional rationale>
```

### 5. Resolve Thread (fixes only)

Only resolve threads that were **actually fixed**. Deferred items stay
unresolved (tracking is in the issue, not the PR thread).

```bash
gh api graphql -f query='
  mutation($threadId:ID!) {
    resolveReviewThread(input:{threadId:$threadId}) { thread { isResolved } }
  }' -f threadId="PRRT_kwDO..."
```

Bulk resolve multiple threads in one call:

```bash
gh api graphql -f query='
  mutation {
    r1: resolveReviewThread(input:{threadId:"PRRT_..."}) { thread { isResolved } }
    r2: resolveReviewThread(input:{threadId:"PRRT_..."}) { thread { isResolved } }
    r3: resolveReviewThread(input:{threadId:"PRRT_..."}) { thread { isResolved } }
  }'
```

### 6. Defer Larger Items (P1 / conceptual)

When a comment raises a valid concern that is too large to fix inline:

#### a) Create OpenSpec Spec

File: `openspec/specs/<kebab-case-name>.spec.md`

````markdown
# <Title>

**Status:** Draft · **Priority:** P1 · **Created:** <YYYY-MM-DD>

---

## Background

<why this matters, what triggered it, which PR comment>

## Requirements

### R1: <requirement>

**Given** <context>
**When** <action>
**Then** <expected behavior>

### R2: ...

## Design

### Approach

<how to solve it, incremental steps>

### Affected files

| File           | Change        |
| -------------- | ------------- |
| `src/File.cpp` | <description> |

### API changes

```cpp
// New or modified signatures
```
````

## Tasks

- [ ] **T1:** <independently completable step>
- [ ] **T2:** ...

## Test Requirements

- <how to verify each requirement>

````

#### b) Create OpenSpec Change Marker

File: `openspec/changes/p1-<kebab-case-name>/.openspec.yaml`

```yaml
schema: spec-driven
created: <YYYY-MM-DD>
title: <Title>
priority: P1
spec: <spec-filename>.spec.md
````

#### c) Create GitHub Issue

```bash
gh issue create \
  --label "P1,security,enhancement" \
  --title "<Title>" \
  --body "## Background

<from spec Background>

## Requirements

<R1, R2, ...>

## OpenSpec

\`openspec/specs/<spec-filename>.spec.md\`

Originally raised in PR #N review comment."
```

#### d) Commit Spec + Marker

```bash
git add openspec/
git commit -m "docs: add OpenSpec for <title>

Addresses PR #N review comment.
"
```

### 7. After All Comments

Re-request review to trigger another review pass:

```bash
# Re-request Codex review
gh api "repos/:owner/:repo/pulls/N/requested-reviewers" \
  --method POST \
  --field reviewers='["github-actions[bot]"]'

# Or re-request human reviewer
gh pr edit N --add-reviewer "<username>"
```

## gh Helper: Get Thread ID for Path + Line

```bash
thread_id_for() {
  local path="$1" line="$2" pr="$3"
  gh api "repos/:owner/:repo/pulls/$pr/comments" \
    --jq ".[] | select(.path == \"$path\" and .line == $line) | .thread_id"
}
```

## gh Helper: List All Unresolved Threads

```bash
list_unresolved() {
  local pr="$1"
  gh api graphql -f query='
    query($owner:String!,$repo:String!,$pr:Int!) {
      repository(owner:$owner,name:$repo) {
        pullRequest(number:$pr) {
          reviewThreads(first:100) {
            nodes {
              id
              isResolved
              isOutdated
              path
              line
              comments(first:5) { nodes { body } }
}}}}}}' -f owner="$OWNER" -f repo="$REPO" -F pr="$pr" \
    | jq '.data.repository.pullRequest.reviewThreads.nodes |
           map(select(.isResolved == false and .isOutdated == false))'
}
```

## Real Examples from Project History

### Fix + Resolve

Review comment: "AP mode should require auth when device is configured"

```bash
# Fix applied to src/WebPortal.cpp - isClientAuthenticated()
git commit -m "fix(web): require auth in AP mode when configured

Addresses PR #77 review comment."

# Reply
gh api graphql -f query='
  mutation($body:String!,$threadId:ID!) {
    addPullRequestReviewThreadReply(input:{
      body:$body,pullRequestId:$prId,threadId:$threadId
    }) { comment { url } }
  }' -f body="✅ Fixed in \`30064fc\`: AP fallback auth now requires password when device is configured" \
  -f threadId="PRRT_kwDOClqq8M6HK6Sk"

# Resolve
gh api graphql -f query='
  mutation($threadId:ID!) {
    resolveReviewThread(input:{threadId:$threadId}) { thread { isResolved } }
  }' -f threadId="PRRT_kwDOClqq8M6HK6Sk"
```

### Defer + OpenSpec + Issue

Review comment: "OTA downloads should verify binary signatures"

```bash
# Create openspec/specs/ota-binary-verification.spec.md
# Create openspec/changes/p1-ota-binary-verification/.openspec.yaml

gh issue create \
  --label "P1,security,enhancement" \
  --title "OTA Binary Verification" \
  --body "..."

git add openspec/
git commit -m "docs: add OpenSpec for OTA binary verification

Addresses PR #77 review comment."

# Reply (no resolve)
gh api graphql -f query='...' \
  -f body="📋 Tracked in #78: OTA Binary Verification"
```

## Related

- Project skill: `conventional-commits` — commit message format and scopes
- Project skill: `iot-security` — security hardening patterns
- Project file: `openspec/config.yaml` — OpenSpec project configuration
- `gh pr` documentation: <https://cli.github.com/manual/gh_pr>
- `gh api` documentation: <https://cli.github.com/manual/gh_api>
