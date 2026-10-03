# Deskbot Fleet

`selected_boards.json` controls OTA rollout.

Use `{"rollout":"all","boards":["all"]}` for every eligible board. Use `{"rollout":"selected","boards":["XIAO-... "]}` for a controlled rollout. Board IDs are printed by the firmware `status` command and exposed through `/api/status`.

A board updates only when the device type matches, the published semantic version is newer, the board is selected, the HTTPS manifest is valid, and the OTA partition is available.
