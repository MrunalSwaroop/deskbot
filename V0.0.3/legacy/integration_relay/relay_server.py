#!/usr/bin/env python3
"""Prototype relay scaffold for Rocky-Wall-E external events.

The relay is intentionally separate from the UNO sketch. A production deployment
must use HTTPS, authentication, durable storage, signed webhook verification,
and an outbound device connection or authenticated device polling.

Prototype environment:
  export RELAY_SECRET='change-me'
  export ROBOT_URL='http://192.168.1.123'
  export MANUS_API_KEY='your-api-key'
  # Optional after the first task is created:
  # export MANUS_TASK_ID='task-id'
  python3 relay_server.py

Endpoints:
  POST /github/webhook  GitHub push/build-style event
  POST /manus/webhook   Manus event adapter placeholder
  POST /manus/ask      Send a prompt to Manus and forward the reply to the robot
  POST /alexa           Alexa Custom Skill fulfillment adapter
  POST /notify          local test notification
"""

from __future__ import annotations

import hashlib
import hmac
import os
import time
from collections import deque
from typing import Any
from urllib.parse import quote

import requests
from flask import Flask, jsonify, request

app = Flask(__name__)
RELAY_SECRET = os.environ.get("RELAY_SECRET", "change-me")
ROBOT_URL = os.environ.get("ROBOT_URL", "http://192.168.1.123").rstrip("/")
MANUS_API_KEY = os.environ.get("MANUS_API_KEY", "")
MANUS_TASK_ID = os.environ.get("MANUS_TASK_ID", "")
MANUS_API_BASE = "https://api.manus.ai"
last_presence_checkin = 0.0
notifications: deque[str] = deque(maxlen=32)


def valid_shared_secret() -> bool:
    supplied = request.headers.get("X-Relay-Secret", "")
    return hmac.compare_digest(supplied, RELAY_SECRET)


def valid_github_signature() -> bool:
    signature = request.headers.get("X-Hub-Signature-256", "")
    expected = "sha256=" + hmac.new(
        RELAY_SECRET.encode(), request.get_data(), hashlib.sha256
    ).hexdigest()
    return hmac.compare_digest(signature, expected)


def queue(message: str) -> None:
    message = " ".join(message.split())[:180]
    if message:
        notifications.append(message)
        # Prototype-only LAN forwarding. Replace this with authenticated outbound
        # device delivery or MQTT/WebSocket in production.
        try:
            requests.get(f"{ROBOT_URL}/cmd/notify/{message.replace(' ', '%20')}", timeout=3)
        except requests.RequestException:
            pass


def alexa_response(text: str, should_end: bool = True) -> dict[str, Any]:
    return {
        "version": "1.0",
        "response": {
            "outputSpeech": {"type": "PlainText", "text": text},
            "shouldEndSession": should_end,
        },
    }


def manus_headers() -> dict[str, str]:
    return {"x-manus-api-key": MANUS_API_KEY, "Content-Type": "application/json"}


def extract_manus_reply(data: dict[str, Any]) -> str:
    for event in data.get("messages", []):
        if event.get("type") == "assistant_message":
            content = event.get("assistant_message", {}).get("content", "")
            if isinstance(content, str) and content.strip():
                return content.strip()
    return ""


def ask_manus(prompt: str) -> str:
    """Use one persistent Manus task, creating it on the first request."""
    global MANUS_TASK_ID
    if not MANUS_API_KEY:
        raise RuntimeError("MANUS_API_KEY is not configured on the relay")

    message = {"content": prompt[:2000]}
    if MANUS_TASK_ID:
        response = requests.post(
            f"{MANUS_API_BASE}/v2/task.sendMessage",
            headers=manus_headers(),
            json={"task_id": MANUS_TASK_ID, "message": message},
            timeout=30,
        )
    else:
        response = requests.post(
            f"{MANUS_API_BASE}/v2/task.create",
            headers=manus_headers(),
            json={
                "message": message,
                "title": "Rocky-Wall-E desk buddy",
                "agent_profile": "manus-1.6-lite",
                "interactive_mode": False,
                "hide_in_task_list": True,
                "share_visibility": "private",
            },
            timeout=30,
        )
    response.raise_for_status()
    created = response.json()
    if not created.get("ok", True):
        raise RuntimeError(str(created.get("error", "Manus request failed")))
    MANUS_TASK_ID = created.get("task_id", MANUS_TASK_ID)
    if not MANUS_TASK_ID:
        raise RuntimeError("Manus did not return a task_id")

    for _ in range(30):
        time.sleep(1.5)
        messages = requests.get(
            f"{MANUS_API_BASE}/v2/task.listMessages",
            headers={"x-manus-api-key": MANUS_API_KEY},
            params={"task_id": MANUS_TASK_ID, "order": "desc", "limit": 20},
            timeout=30,
        )
        messages.raise_for_status()
        reply = extract_manus_reply(messages.json())
        if reply:
            return reply[:180]
    raise TimeoutError("Manus did not finish within the relay wait window")


@app.get("/health")
def health():
    return jsonify({"ok": True, "queued": len(notifications)})


@app.post("/notify")
def local_notify():
    if not valid_shared_secret():
        return jsonify({"ok": False, "error": "unauthorized"}), 401
    payload = request.get_json(silent=True) or {}
    message = str(payload.get("message", ""))
    queue(message)
    return jsonify({"ok": True, "queued": message[:180]})


@app.post("/manus/ask")
def manus_ask():
    if not valid_shared_secret():
        return jsonify({"ok": False, "error": "unauthorized"}), 401
    payload = request.get_json(silent=True) or {}
    prompt = str(payload.get("prompt", "")).strip()
    personality = str(payload.get("personality", "calm")).strip().lower()
    if not prompt:
        return jsonify({"ok": False, "error": "prompt is required"}), 400
    personality_guidance = {
        "engineer": "Answer as an original Einstein-inspired lab thinker: precise, inventive, humble, and safety-first. Do not imitate Einstein or claim to be him.",
        "spartan": "Answer as an original wise Spartan mentor: disciplined, direct, courageous, practical, and motivating. Keep the motto 'If it is man-made, I can make it' available when appropriate.",
        "musical": "Answer as a warm, lightly playful desk companion with a gentle rhythm. Do not imitate a film or real person's voice.",
        "quiet": "Answer calmly and briefly, without sounding needy.",
        "calm": "Answer calmly, warmly, and practically.",
    }.get(personality, "Answer calmly, warmly, and practically.")
    prompt = personality_guidance + " Keep the final response concise for a desk robot. User request: " + prompt
    try:
        reply = ask_manus(prompt)
        requests.get(f"{ROBOT_URL}/reply/{quote(reply, safe='')}", timeout=5)
        return jsonify({"ok": True, "reply": reply})
    except (requests.RequestException, RuntimeError, TimeoutError) as exc:
        return jsonify({"ok": False, "error": str(exc)}), 502


@app.post("/presence")
def presence_event():
    """Future camera adapter: trigger at most one check-in per 10 minutes."""
    global last_presence_checkin
    if not valid_shared_secret():
        return jsonify({"ok": False, "error": "unauthorized"}), 401
    payload = request.get_json(silent=True) or {}
    present = bool(payload.get("present", False))
    now = time.time()
    if present and now - last_presence_checkin >= 600:
        try:
            requests.get(f"{ROBOT_URL}/cmd/checkin%20now", timeout=5)
            last_presence_checkin = now
            return jsonify({"ok": True, "action": "check-in triggered"})
        except requests.RequestException as exc:
            return jsonify({"ok": False, "error": str(exc)}), 502
    return jsonify({"ok": True, "action": "ignored", "present": present})


@app.post("/github/webhook")
def github_webhook():
    if not valid_github_signature():
        return jsonify({"ok": False, "error": "invalid signature"}), 401
    payload = request.get_json(silent=True) or {}
    event = request.headers.get("X-GitHub-Event", "unknown")
    repo = payload.get("repository", {}).get("full_name", "repository")
    if event == "push":
        commits = len(payload.get("commits", []))
        queue(f"Code updated in {repo}; {commits} new commit(s).")
    elif event in {"workflow_run", "check_run"}:
        status = payload.get("action", payload.get("workflow_run", {}).get("conclusion", "changed"))
        queue(f"Code build status changed for {repo}: {status}.")
    return jsonify({"ok": True})


@app.post("/manus/webhook")
def manus_webhook():
    # Replace this shared-secret check with Manus webhook public-key signature
    # verification when the production webhook schema and key are configured.
    if not valid_shared_secret():
        return jsonify({"ok": False, "error": "unauthorized"}), 401
    payload = request.get_json(silent=True) or {}
    detail = payload.get("task_stopped", payload.get("message", payload))
    summary = str(detail)[:160]
    queue(f"Manus update: {summary}")
    return jsonify({"ok": True})


@app.post("/alexa")
def alexa():
    if not valid_shared_secret():
        return jsonify(alexa_response("I could not authenticate that request.")), 401
    payload = request.get_json(silent=True) or {}
    request_data = payload.get("request", {})
    intent = request_data.get("intent", {})
    name = intent.get("name", "")
    slots = intent.get("slots", {})

    if name == "DanceIntent":
        try:
            requests.get(f"{ROBOT_URL}/cmd/dance%20rocky", timeout=3)
        except requests.RequestException:
            return jsonify(alexa_response("I cannot reach Rocky right now."))
        return jsonify(alexa_response("Starting a small desk dance."))

    if name == "LookIntent":
        direction = str(slots.get("Direction", {}).get("value", "center")).lower()
        if direction not in {"left", "right", "center"}:
            direction = "center"
        try:
            requests.get(f"{ROBOT_URL}/cmd/look%20{direction}", timeout=3)
        except requests.RequestException:
            return jsonify(alexa_response("I cannot reach Rocky right now."))
        return jsonify(alexa_response(f"Looking {direction}."))

    if name == "NotificationIntent":
        return jsonify(alexa_response("I will read the next notification when it is available."))

    return jsonify(alexa_response("I can look left or right, dance, and announce notifications."))


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5060, debug=False)
