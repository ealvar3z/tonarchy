# Releasing (GPLv2 Compliance Checklist)

Use this checklist for every public binary/ISO release.

## 1. Tag a Source Snapshot

- Create a git tag for the exact source commit used to build release artifacts.
- Ensure `LICENSE`, `NOTICE.md`, and `CHANGELOG.md` are present in that tag.

## 2. Build Artifacts from Tagged Source

- Build from a clean checkout of the tagged commit.
- Record the exact commands used for reproducibility.

## 3. Publish Artifacts With Matching Source

- If publishing an ISO/binary, publish corresponding source for the same tag.
- Keep build scripts/config in source publication (`Makefile`, `flake.nix`, `iso/`, etc.).
- Include or link `LICENSE` and `NOTICE.md` with release notes.

## 4. Verify User Access

- Confirm release page links to:
  - source tag/commit
  - checksums
  - license notice

## 5. Keep Notices Intact

- Do not remove upstream attribution.
- Add fork-specific attribution in `NOTICE.md` when needed.
