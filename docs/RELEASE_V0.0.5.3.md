# Deskbot v0.0.5.3 — Wi-Fi Profiles, Remote OLED, and Versioned OTA

This release is built on the verified v0.0.5.2 base. It adds two saved Wi-Fi profiles, a five-minute setup-AP fallback, a remote OLED mirror, and retained versioned OTA artifacts.

## 1. Apply the source change on Windows

From the existing Deskbot repository:

```powershell
cd -LiteralPath "C:\Users\msperavali\OneDrive - VE Commercial Vehicles Ltd\Mrunal\Deskbot"
git status --short --branch
python tools\check_release.py
```

Copy the v0.0.5.3 source bundle over the existing repository files, but **do not copy** a local `ota_target.h`, `secrets.h`, Arduino build folder, or `.git` directory. The active sketch remains:

```text
firmware\xiao_esp32s3_sense\xiao_esp32s3_sense.ino
```

Run:

```powershell
python tools\check_release.py
git diff --check
git add .
git diff --cached --check
git commit -m "Deskbot v0.0.5.3 dual Wi-Fi and versioned OTA"
git push origin main
```

The **Publish Deskbot OTA** workflow will compile with ESP32 Arduino core `3.3.7`, publish the current manifest, update `ota/catalog.json`, retain the image under `ota/releases/<version>/`, and create a GitHub Release `v0.0.5.3`.

## 2. Confirm GitHub before touching the board

In GitHub Actions, wait for both **Build all boards** and **Publish Deskbot OTA** to show green. Then run:

```powershell
Invoke-RestMethod "https://MrunalSwaroop.github.io/deskbot/ota/xiao-esp32s3-manifest.json"
Invoke-RestMethod "https://MrunalSwaroop.github.io/deskbot/ota/catalog.json"
```

The manifest should report `version : 0.0.5.3`. The catalog should contain the new version and older GitHub Release versions. The GitHub Releases page should show `Deskbot v0.0.5.3` with `XIAO-ESP32S3.bin`.

## 3. Let the running board update normally

Keep the robot stationary and motor power off. The board must already be running an OTA-capable image and connected to Wi-Fi.

1. Open the dashboard at the printed device IP.
2. Confirm the Firmware and OTA section shows the current version and a catalog selector.
3. Leave the selector at **Latest release**.
4. Click **Restart and check OTA**.
5. Watch Serial Monitor. Expected sequence:

```text
OTA bootstrap: manifest version=0.0.5.3
OTA bootstrap: downloading XIAO firmware version 0.0.5.3
OTA state: downloading ...%
OTA state: installing 100%
OTA state: rebooting 100%
```

6. Wait for the board to reboot and reconnect. Open the new IP if the network assigned a different address.
7. Confirm `/api/status` reports firmware `0.0.5.3` and OTA state `up_to_date`.

## 4. Configure two Wi-Fi profiles

Open the dashboard’s **Configure two networks** link, or connect to `Rocky-XIAO-Setup` and open:

```text
http://192.168.4.1/wifi
```

Enter:

- Profile 1: the preferred 2.4 GHz network and password.
- Profile 2: an optional backup 2.4 GHz network and password.

Click **Save profiles and reconnect**. The board reboots and tries profile 1, then profile 2. The dashboard badge shows the active profile index. If a password field is left blank while keeping the same SSID, its stored password is preserved.

If both saved networks remain unavailable continuously for five minutes, the board starts `Rocky-XIAO-Setup` at `192.168.4.1`. Connect to that AP, save the replacement network, and allow the board to reboot.

## 5. Verify remote OLED monitoring

The dashboard includes a pixelated OLED preview. It refreshes once per second from:

```text
http://XIAO_IP/oled.svg
```

The same mirror is visible on the setup page at `http://192.168.4.1/wifi`. This is a local-network monitor; do not expose the XIAO dashboard or setup AP to the public internet.

Test the display and state controls:

```text
face idle
face listening
face thinking
face speaking
personality isabella
personality rocky
oled invert toggle
```

The remote mirror should change when the physical OLED changes.

## 5A. Verify the dashboard change log

At the top of the connected dashboard, confirm the **What changed** card shows:

- release badge `v0.0.5.3`;
- a release summary;
- the bullet list of changes included in this firmware;
- an **Open changes JSON** link.

The machine-readable endpoint is:

```text
http://XIAO_IP/api/changes
```

It returns the running release version, title, summary, and change list. This lets you monitor what is installed without opening the repository.

## 6. Intentional upgrade or downgrade

The board normally installs only a newer manifest version. To select a specific retained version:

```text
ota target 0.0.5.2
restart
```

Or use the dashboard catalog selector and click **Set target**, then **Restart and check OTA**. The target is stored in NVS and survives reboot until the selected image is installed successfully. To return to normal newest-release behavior:

```text
ota latest
restart
```

The catalog entries point to immutable GitHub Release assets. This keeps older versions selectable even after later Pages deployments.

## 7. Hardware verification after the update

Run these tests in order, with the robot lifted for motor tests:

```text
status
test oled
test servo
pan 0
pan 90
pan 180
test mic
mic monitor
audio volume 20
audio tone 440 500
mic loopback 10
audio stop
motor test
motor stop
```

Verify the camera only when its ribbon cable is installed. The camera feature remains compiled into the firmware; a removed camera may report `Detected camera not supported`, which is expected while the module is disconnected.

## 8. Recovery

If the board does not boot after an OTA image, use the saved GitHub Release asset for the last known-good version or reflash the USB-only build through Arduino IDE. Do not delete old releases and do not reuse a version number for a correction. Publish a higher four-part version instead.

Never commit `ota_target.h`, `secrets.h`, Wi-Fi passwords, API keys, build output, or serial logs containing credentials.

## References

- [GitHub Actions](https://docs.github.com/en/actions)
- [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
- [GitHub Pages](https://docs.github.com/en/pages)
- [Seeed XIAO ESP32-S3 Sense getting started](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
