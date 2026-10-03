# Deskbot Tools

Run `python3 tools/check_release.py` before committing a release. The checker validates the active board profile, firmware, module registry, fleet policy, hardware diagrams, semantic version, and private-file exclusions.

The repository intentionally does not commit local OTA targets, Wi-Fi credentials, API keys, build folders, or compiled binaries.
