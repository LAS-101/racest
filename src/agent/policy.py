"""RACEST's autonomous typing decisions. Uses only Python's standard library."""
from dataclasses import dataclass
import math


@dataclass(frozen=True)
class Action:
    text: str = ""
    backspace: bool = False

    def command(self) -> str:
        if self.backspace:
            return "backspace"
        return f"type {self.text}" if self.text else "idle"

    def as_dict(self) -> dict:
        return {"text": self.text, "backspace": self.backspace}


class TypingAgent:
    def __init__(self, wpm: float = 120):
        self.set_wpm(wpm)
        self.keystrokes = 0

    def set_wpm(self, wpm: float):
        if not math.isfinite(wpm) or not 1 <= wpm <= 600:
            raise ValueError("Agent speed must be between 1 and 600 WPM")
        self.wpm = float(wpm)
        self._wait = 0.0

    def reset(self):
        self._wait = 0.0
        self.keystrokes = 0

    def next_input(self, observation: dict) -> Action:
        if observation["state"] != "playing" or not observation.get("enabled", True):
            return Action()
        dt = observation["dt"]
        if not math.isfinite(dt) or dt < 0:
            return Action()
        interval = 12 / self.wpm  # Five characters per standard word.
        self._wait -= min(dt, interval)
        if self._wait > 0.000001:
            return Action()

        words = observation["words"]
        locked = next((word for word in words if word["targeted"]), None)
        action = Action()
        if locked:
            if locked["errored"]:
                action = Action(backspace=True)
            elif 0 <= locked["typed"] < len(locked["text"]):
                action = Action(text=locked["text"][locked["typed"]])
        else:
            screen = observation["screen"]
            candidates = []
            for index, word in enumerate(words):
                text = word["text"]
                if not text or word["speed"] <= 0:
                    continue
                # Require the whole word to fit, conservatively estimating width.
                if word["x"] + len(text) * screen["font_size"] > screen["width"]:
                    continue
                # First-letter selection locks the leftmost match, breaking ties
                # in spawn order. A faster matching word may be unreachable.
                if any(other["text"] and other["text"][0] == text[0] and
                       (other["x"] < word["x"] or (other["x"] == word["x"] and other_index < index))
                       for other_index, other in enumerate(words)):
                    continue
                deadline = (word["x"] - screen["danger_x"]) / word["speed"]
                slack = deadline - len(text) * interval
                candidates.append((slack, index, text))
            if candidates:
                action = Action(text=min(candidates)[2][0])

        if action.text or action.backspace:
            self.keystrokes += 1
            self._wait += interval
        else:
            self._wait = 0.0  # Never bank idle time for a typing burst.
        return action
