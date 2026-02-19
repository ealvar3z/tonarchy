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
  - [ ] (Optional later) `profiles/manifests/` for install profile definitions.
- [x] Move `walls/packages/*.txt` to `profiles/packages/*.txt`.
- [x] Resolve duplicate wallpaper placement (`walls/wall1.jpg` vs `assets/wallpapers/wall1.jpg`).
- [x] Update references in docs/build/runtime paths.
- [x] Verify repository state after move (`rg walls/` should show no stale refs except intentional notes).

## Phase 2: Ham Package Groups

- [ ] Add `profiles/packages/ham_core.txt`.
- [ ] Add `profiles/packages/ham_digital_modes.txt`.
- [ ] Add `profiles/packages/ham_logging.txt`.
- [ ] Add `profiles/packages/ham_audio_serial_tools.txt`.
- [ ] Add `profiles/packages/ham_packet_modes.txt`.
- [ ] Add `profiles/packages/ham_sdr.txt` (exclude `hackrf` by default).
- [ ] Optional: add `profiles/packages/ham_sdr_hackrf.txt` as opt-in hardware extras.

## Phase 3: Ham Profile Definition (XFCE Base)

- [ ] Define initial profile composition (data-only dry run first):
  - [ ] `base`
  - [ ] `display_xorg`
  - [ ] XFCE group(s)
  - [ ] all selected `ham_*` groups
- [ ] Generate and review deduplicated package list.
- [ ] Review package names for Arch repos and adjust mismatches.

## Phase 4: Installer Integration

- [ ] Replace hardcoded package-only mode path with package-group/profile-driven resolution.
- [ ] Keep existing Beginner/Oxidized behavior working during transition.
- [ ] Add new ham XFCE install option to menu.
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
