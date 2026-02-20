# Plan

Implementation plan for ham-radio profile support and repository cleanup.

## Phase 0: Project Hygiene

- [x] Create `CHANGELOG.md`.
- [x] Create `PLAN.md`.
- [x] Add a short section in `README.org` pointing to `CHANGELOG.md` and `PLAN.md`.

## Phase 1: Directory Refactor (`walls` -> clear structure)

- [ ] Define target structure:
  - [x] `profiles/packages/` for package-group manifests.
  - [x] `assets/wallpapers/` for wallpaper assets only.
  - [x] (Optional later) `profiles/manifests/` for install profile definitions.
- [x] Move `walls/packages/*.txt` to `profiles/packages/*.txt`.
- [x] Resolve duplicate wallpaper placement (`walls/wall1.jpg` vs `assets/wallpapers/wall1.jpg`).
- [x] Update references in docs/build/runtime paths.
- [x] Verify repository state after move (`rg walls/` should show no stale refs except intentional notes).

## Phase 2: Ham Package Groups

- [x] Add `profiles/packages/ham_core.txt`.
- [x] Add `profiles/packages/ham_digital_modes.txt`.
- [x] Add `profiles/packages/ham_logging.txt`.
- [x] Add `profiles/packages/ham_audio_serial_tools.txt`.
- [x] Add `profiles/packages/ham_packet_modes.txt`.
- [x] Add `profiles/packages/ham_sdr.txt` (exclude `hackrf` by default).
- [x] Optional: add `profiles/packages/ham_sdr_hackrf.txt` as opt-in hardware extras.

## Phase 3: Ham Profile Definition (XFCE Base)

- [x] Define initial profile composition (data-only dry run first):
  - [x] `base`
  - [x] `display_xorg`
  - [x] XFCE group(s)
  - [x] all selected `ham_*` groups
- [x] Generate and review deduplicated package list.
- [x] Review package names for Arch repos and adjust mismatches.

## Phase 4: Installer Integration

- [x] Replace hardcoded package-only mode path with package-group/profile-driven resolution.
- [x] Keep existing Beginner/Oxidized behavior working during transition.
- [x] Add new ham XFCE install option to menu.
- [ ] Wire selected profile to `pacstrap` package resolution.
- [ ] Ensure post-install config for ham profile reuses XFCE setup path.

## Phase 5: Validation

- [ ] Build binaries: `make`, `make static`, `make build_iso`.
- [ ] Validate installer startup from ISO in VM.
- [ ] Validate ham profile selection and package installation path.
- [ ] Smoke-test Wi-Fi setup and first boot UX.

## Phase 6: Documentation and Release Notes

- [ ] Update `README.org` with profile model and ham profile details.
- [ ] Document optional `hackrf` package group.
- [ ] Add final changelog entries for completed phases.
