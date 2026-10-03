#!/usr/bin/env python3
"""Temporary audio/text helper for the non-destructive UNO R4 architecture.

The UNO R4 calls the LLM. This laptop helper only provides temporary input and
output until a microphone and speaker are added to the robot.

Text mode:
  python3 laptop_audio_bridge.py --robot http://192.168.1.123

One request:
  python3 laptop_audio_bridge.py --robot http://192.168.1.123 --text "dance"

Optional voice mode requires SpeechRecognition plus a microphone backend:
  python3 -m pip install requests SpeechRecognition PyAudio pyttsx3
  python3 laptop_audio_bridge.py --robot http://192.168.1.123 --voice --personality calm

Speak spontaneous UNO replies/check-ins from the laptop:
  python3 laptop_audio_bridge.py --robot http://192.168.1.123 --watch

The temporary TTS delivery differs by personality: calm is slower and warm,
musical is brighter, engineer is faster and precise, and quiet is slower/lower.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from urllib.parse import quote

import requests


def robot_get(robot_url: str, path: str) -> str:
    response = requests.get(robot_url.rstrip("/") + path, timeout=15)
    response.raise_for_status()
    return response.text


def command(robot_url: str, value: str) -> str:
    return robot_get(robot_url, "/cmd/" + quote(value, safe=""))


def extract_reply(raw: str) -> str:
    """Extract the assistant content from the raw OpenAI-compatible response."""
    body = raw.split("\r\n\r\n", 1)[-1]
    try:
        data = json.loads(body)
        choices = data.get("choices", [])
        if choices:
            message = choices[0].get("message", {})
            content = message.get("content", "")
            if isinstance(content, str) and content.strip():
                try:
                    structured = json.loads(content)
                    reply = structured.get("reply", "")
                    if isinstance(reply, str) and reply.strip():
                        return reply.strip()
                except json.JSONDecodeError:
                    pass
                return content.strip()
    except json.JSONDecodeError:
        pass
    return "I received a response, but I could not read its words yet."


def ask(robot_url: str, text: str, speak: bool = True, personality: str = "calm") -> str:
    command(robot_url, "state listening")
    print("LISTENING:", text)
    # /input makes the UNO show THINKING, call the remote LLM, validate the
    # result, and store the raw response at /last.
    robot_get(robot_url, "/input/" + quote(text, safe=""))
    command(robot_url, "state speaking")
    raw = robot_get(robot_url, "/last")
    reply = extract_reply(raw)
    print("ROCKY:", reply)
    if speak:
        speak_text(reply, personality)
    command(robot_url, "state idle")
    return reply


def speak_text(text: str, personality: str = "calm") -> None:
    try:
        import pyttsx3  # type: ignore

        engine = pyttsx3.init()
        delivery = {
            "calm": (132, 0.70, ("zira", "samantha", "female")),
            "musical": (148, 0.78, ("zira", "samantha", "female")),
            "engineer": (174, 0.82, ("david", "alex", "male")),
            "quiet": (112, 0.48, ("zira", "samantha", "female")),
        }.get(personality, (132, 0.70, ("zira", "samantha", "female")))
        engine.setProperty("rate", delivery[0])
        engine.setProperty("volume", delivery[1])
        wanted = delivery[2]
        for voice in engine.getProperty("voices") or []:
            identity = f"{voice.id} {voice.name}".lower()
            if any(keyword in identity for keyword in wanted):
                engine.setProperty("voice", voice.id)
                break
        engine.say(text)
        engine.runAndWait()
    except Exception as exc:
        print("TTS unavailable; read the reply above:", exc, file=sys.stderr)


def voice_loop(robot_url: str, personality: str = "calm") -> None:
    try:
        import speech_recognition as sr  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "Voice mode needs SpeechRecognition and PyAudio. "
            "Install with: python3 -m pip install SpeechRecognition PyAudio pyttsx3"
        ) from exc

    recognizer = sr.Recognizer()
    with sr.Microphone() as microphone:
        print("Calibrating microphone...")
        recognizer.adjust_for_ambient_noise(microphone, duration=1)
        print("Voice mode ready. Say 'stop robot' to exit.")
        while True:
            command(robot_url, "state listening")
            try:
                audio = recognizer.listen(microphone, timeout=8, phrase_time_limit=8)
                text = recognizer.recognize_google(audio)
            except sr.WaitTimeoutError:
                command(robot_url, "state idle")
                continue
            except sr.UnknownValueError:
                command(robot_url, "state idle")
                print("I could not understand that.")
                continue
            except sr.RequestError as exc:
                command(robot_url, "state error")
                print("Speech recognition service error:", exc)
                continue

            print("YOU:", text)
            if text.lower().strip() in {"stop robot", "exit robot", "quit robot"}:
                command(robot_url, "state sleep")
                break
            ask(robot_url, text, speak=True, personality=personality)
            time.sleep(0.2)


def watch_loop(robot_url: str, speak: bool = True) -> None:
    """Speak new spontaneous UNO replies, including random check-ins."""
    try:
        state = json.loads(robot_get(robot_url, "/api/status"))
        last_sequence = int(state.get("seq", 0))
    except (ValueError, json.JSONDecodeError):
        last_sequence = 0
    print("Watching for Rocky replies and check-ins. Ctrl-C exits.")
    while True:
        try:
            state = json.loads(robot_get(robot_url, "/api/status"))
            sequence = int(state.get("seq", 0))
            if sequence != last_sequence:
                last_sequence = sequence
                reply = extract_reply(robot_get(robot_url, "/last"))
                personality = str(state.get("mode", "calm"))
                print("ROCKY:", reply)
                command(robot_url, "state speaking")
                if speak:
                    speak_text(reply, personality)
                command(robot_url, "state idle")
            time.sleep(2)
        except KeyboardInterrupt:
            print()
            break
        except requests.RequestException as exc:
            print("Robot temporarily unavailable:", exc, file=sys.stderr)
            time.sleep(3)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--robot", required=True, help="UNO IP, e.g. http://192.168.1.123")
    parser.add_argument("--text", help="Send one text request and exit")
    parser.add_argument("--voice", action="store_true", help="Use laptop microphone and optional TTS")
    parser.add_argument("--watch", action="store_true", help="Watch for spontaneous replies/check-ins")
    parser.add_argument("--personality", choices=("calm", "musical", "engineer", "quiet"), default="calm")
    parser.add_argument("--no-speak", action="store_true", help="Print replies without laptop TTS")
    args = parser.parse_args()

    robot_get(args.robot, "/health")
    if args.text:
        ask(args.robot, args.text, speak=not args.no_speak, personality=args.personality)
    elif args.watch:
        watch_loop(args.robot, speak=not args.no_speak)
    elif args.voice:
        voice_loop(args.robot, personality=args.personality)
    else:
        print("Text mode. Type a request; Ctrl-C exits.")
        while True:
            try:
                text = input("YOU: ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break
            if text:
                ask(args.robot, text, speak=not args.no_speak, personality=args.personality)


if __name__ == "__main__":
    main()
