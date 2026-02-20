# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Project-level planning docs:
  - `CHANGELOG.md` for milestone tracking.
  - `PLAN.md` for step-by-step implementation tracking.
- Repository tracking section in `README.org` pointing to planning docs.
- Ham-radio package groups under `profiles/packages/`:
  - `ham_core.txt`
  - `ham_digital_modes.txt`
  - `ham_logging.txt`
  - `ham_audio_serial_tools.txt`
  - `ham_packet_modes.txt`
  - `ham_sdr.txt` (without `hackrf` by default)
  - `ham_sdr_hackrf.txt` (optional hardware extras)
- Added `rtl_433` to `ham_core.txt` for RTL-SDR utility coverage.
- Added optional ADS-B group `profiles/packages/ham_rtl_adsb.txt` (`dump1090_rs`).
- Added XFCE profile model artifacts:
  - `profiles/packages/de_xfce.txt`
  - `profiles/manifests/ham_xfce.conf`
  - `profiles/manifests/ham_xfce.packages.txt`
  - `profiles/manifests/ham_xfce.aur-packages.txt`
  - `profiles/manifests/ham_xfce.validation.md`

### Changed
- Refactored package-manifest layout:
  - moved `walls/packages/*.txt` -> `profiles/packages/*.txt`
  - removed duplicate `walls/wall1.jpg` (kept `assets/wallpapers/wall1.jpg`)
- Adjusted package naming for current Arch repos:
  - removed `gpsd-clients` from `ham_core.txt` (covered by `gpsd`)
  - changed `soapysdr-module-rtlsdr` -> `soapyrtlsdr` in `ham_sdr.txt`
  - changed `dump1090` -> `dump1090_rs` in `ham_rtl_adsb.txt`
- Split ham XFCE definition into:
  - official repo install path (`package_groups`)
  - AUR-intended groups (`aur_package_groups`) for later tooling support
- Installer package selection refactor (`src/tonarchy.c`):
  - replaced hardcoded monolithic package strings with package-group composition
  - added profile package resolver with de-duplication for current Beginner/Oxidized modes
  - added compatibility validation to ensure Beginner/Oxidized mode package sets include required mode-specific packages

### Planned
- Introduce ham-radio package groups:
  - `ham_core`
  - `ham_digital_modes`
  - `ham_logging`
  - `ham_audio_serial_tools`
  - `ham_packet_modes`
  - `ham_sdr` (without `hackrf` by default)
- Use XFCE as the base desktop for the ham profile.
- Refactor directory layout to replace ambiguous `walls/` naming with clearer profile/package structure.
