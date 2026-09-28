#!/usr/bin/env python3
"""Validate the repository contract before a Deskbot firmware release."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    Path("VERSION"),
    Path("boards/xiao-esp32s3-sense/board.json"),
    Path("boards/xiao-esp32s3-sense/pinmap.h"),
    Path("fleet/selected_boards.json"),
    Path("firmware/xiao_esp32s3_sense/xiao_esp32s3_sense.ino"),
    Path("firmware/xiao_esp32s3_sense/device_config.h"),
    Path("firmware/xiao_esp32s3_sense/ota_service.cpp"),
    Path("firmware/xiao_esp32s3_sense/ota_service.h"),
    Path("modules/core/face_state.h"),
    Path("modules/core/personality.h"),
    Path("modules/faces/face_renderer.h"),
    Path("modules/faces/isabella_character.h"),
    Path("modules/faces/isabella_state_bitmaps.h"),
    Path("modules/faces/spartan_character.h"),
    Path("modules/faces/rocky_bump_frames.h"),
    Path("modules/faces/rocky_body_language.h"),
    Path("firmware/xiao_esp32s3_sense/secrets.h.example"),
    Path("hardware/wiring/complete_wiring.mmd"),
    Path("CHANGE_SUMMARY.md"),
]


def fail(message: str) -> None:
    print(f"RELEASE CHECK FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    missing = [str(path) for path in REQUIRED if not (ROOT / path).is_file()]
    if missing:
        fail("missing required files: " + ", ".join(missing))

    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        fail(f"VERSION is not semantic x.y.z: {version!r}")

    try:
        board = json.loads((ROOT / "boards/xiao-esp32s3-sense/board.json").read_text())
        fleet = json.loads((ROOT / "fleet/selected_boards.json").read_text())
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON: {exc}")

    expected_fqbn = "esp32:esp32:XIAO_ESP32S3:PSRAM=opi,FlashMode=qio,FlashSize=8M,USBMode=hwcdc,CDCOnBoot=default,UploadMode=default,PartitionScheme=default_8MB"
    if board.get("fqbn") != expected_fqbn:
        fail("active board profile does not use the verified XIAO Sense build settings")
    if board.get("core_version") != "3.3.7":
        fail("active board profile does not pin ESP32 Arduino core 3.3.7")
    if fleet.get("rollout") not in {"all", "selected"}:
        fail("fleet rollout must be all or selected")
    if not isinstance(fleet.get("boards"), list) or not fleet["boards"]:
        fail("fleet boards must be a non-empty list")
    if fleet["rollout"] == "selected" and "all" in fleet["boards"]:
        fail("selected rollout cannot contain the all wildcard")

    if (ROOT / "firmware/xiao_esp32s3_sense/ota_target.h").exists():
        fail("local ota_target.h must never be committed or packaged")

    forbidden = re.compile(r"sk-[A-Za-z0-9_-]{20,}")
    for path in ROOT.rglob("*"):
        if not path.is_file() or path.suffix.lower() in {".png", ".bin", ".elf", ".map", ".zip"}:
            continue
        if "build" in path.parts or ".git" in path.parts:
            continue
        if forbidden.search(path.read_text(encoding="utf-8", errors="ignore")):
            fail(f"possible secret pattern found in {path.relative_to(ROOT)}")

    print(f"RELEASE CHECK OK: Deskbot {version}; rollout={fleet['rollout']}; boards={fleet['boards']}")


if __name__ == "__main__":
    main()
