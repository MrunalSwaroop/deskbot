#!/usr/bin/env python3
"""Local Rocky-Wall-E LLM bridge.

The robot never receives the LLM API key. A laptop on the same Wi-Fi network
calls this service, the service asks an OpenAI-compatible chat API for one
allowed robot command, and then forwards that command to the UNO HTTP server.

Run:
  python3 -m pip install flask openai requests
  export OPENAI_API_KEY='...'
  export ROBOT_URL='http://192.168.1.123'
  python3 robot_llm_bridge.py

Then test:
  curl -X POST http://127.0.0.1:5050/chat \\
       -H 'Content-Type: application/json' \\
       -d '{"text":"look happy and say hello"}'
"""

import json
import os
import re
from urllib.parse import quote

import requests
from flask import Flask, jsonify, request
from openai import OpenAI

app = Flask(__name__)

ROBOT_URL = os.environ.get("ROBOT_URL", "http://192.168.1.123").rstrip("/")
LLM_MODEL = os.environ.get("LLM_MODEL", "gpt-4o-mini")
ALLOWED_COMMANDS = {
    "idle",
    "happy",
    "listen",
    "speak",
    "sleep",
    "surprise",
    "sad",
    "curious",
    "worried",
    "look left",
    "look right",
    "look center",
}

SYSTEM_PROMPT = """You are Rocky-Wall-E, a calm and caring desk robot.
Return JSON only, with exactly one field named command.
Choose exactly one command from this list:
idle, happy, listen, speak, sleep, surprise, sad, curious, worried,
look left, look right, look center.
Do not invent commands. Do not control motors. Keep the personality gentle,
quietly curious, practical, and reassuring. If the request is unclear, choose
idle. A greeting normally maps to happy. A request to pay attention maps to
listen. A request to talk maps to speak. A request to look in a direction maps
to the corresponding look command.
"""


def extract_command(content: str) -> str:
    """Accept strict JSON, then a fenced JSON fallback, then a bare command."""
    text = content.strip()
    candidates = [text]
    fenced = re.search(r"```(?:json)?\s*(\{.*?\})\s*```", text, re.S)
    if fenced:
        candidates.insert(0, fenced.group(1))

    for candidate in candidates:
        try:
            value = json.loads(candidate).get("command", "")
            if isinstance(value, str) and value.strip().lower() in ALLOWED_COMMANDS:
                return value.strip().lower()
        except (json.JSONDecodeError, AttributeError):
            pass

    bare = text.lower().replace("_", " ")
    for command in sorted(ALLOWED_COMMANDS, key=len, reverse=True):
        if command in bare:
            return command
    return "idle"


def ask_llm(user_text: str) -> str:
    client = OpenAI()
    response = client.chat.completions.create(
        model=LLM_MODEL,
        messages=[
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": user_text[:500]},
        ],
        temperature=0.3,
        max_tokens=80,
    )
    return extract_command(response.choices[0].message.content or "")


def send_robot_command(command: str) -> str:
    response = requests.get(
        f"{ROBOT_URL}/cmd/{quote(command, safe='')}", timeout=3
    )
    response.raise_for_status()
    return response.text


@app.get("/health")
def health():
    return jsonify({"ok": True, "robot_url": ROBOT_URL, "model": LLM_MODEL})


@app.post("/command")
def direct_command():
    payload = request.get_json(silent=True) or {}
    command = str(payload.get("command", "")).strip().lower()
    if command not in ALLOWED_COMMANDS:
        return jsonify({"ok": False, "error": "command not allowed"}), 400
    try:
        robot_response = send_robot_command(command)
        return jsonify({"ok": True, "command": command, "robot": robot_response})
    except requests.RequestException as exc:
        return jsonify({"ok": False, "error": f"robot request failed: {exc}"}), 502


@app.post("/chat")
def chat():
    payload = request.get_json(silent=True) or {}
    user_text = str(payload.get("text", "")).strip()
    if not user_text:
        return jsonify({"ok": False, "error": "JSON field 'text' is required"}), 400

    try:
        command = ask_llm(user_text)
        robot_response = send_robot_command(command)
        return jsonify({"ok": True, "input": user_text, "command": command, "robot": robot_response})
    except Exception as exc:  # Keep the prototype response readable during setup.
        return jsonify({"ok": False, "error": str(exc)}), 502


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5050, debug=False)
