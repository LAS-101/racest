import json
import math
import os
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
AGENT_DIR = ROOT / "src/agent"
sys.path.insert(0, str(AGENT_DIR))
from policy import Action, TypingAgent


def word(text="cat", x=300, speed=100, typed=0, errored=False, targeted=False):
    return dict(text=text, x=x, speed=speed, typed=typed, errored=errored, targeted=targeted)


def observe(*words, dt=1, state="playing", enabled=True):
    return dict(state=state, enabled=enabled, dt=dt, words=list(words),
                screen=dict(width=1280, danger_x=95, font_size=27))


class PolicyTests(unittest.TestCase):
    def test_only_visible_words_in_live_games(self):
        agent = TypingAgent()
        for snapshot in (observe(word(x=1304)), observe(word("partial", x=1279)),
                         observe(word(), state="menu"), observe(word(), state="gameover"),
                         observe(word(), enabled=False)):
            self.assertEqual(agent.next_input(snapshot), Action())

    def test_deadlines_account_for_speed_and_typing_time(self):
        agent = TypingAgent()
        self.assertEqual(agent.next_input(observe(word("slow", 300, 40), word("fast", 500, 200))).text, "f")

    def test_duplicate_initials_respect_game_lock_rules(self):
        agent = TypingAgent()
        self.assertEqual(agent.next_input(observe(word("cat", 300), word("code", 500, 300))).text, "c")
        agent.reset()
        self.assertEqual(agent.next_input(observe(word("cat", 500, 10), word("code", 500, 300),
                                                 word("fast", 400))).text, "f")

    def test_locked_target_and_error_recovery(self):
        agent = TypingAgent()
        locked = word("space", 700, typed=2, targeted=True)
        self.assertEqual(agent.next_input(observe(word("danger", 100), locked)).text, "a")
        agent.reset()
        locked["errored"] = True
        self.assertTrue(agent.next_input(observe(locked)).backspace)

    def test_typing_cadence(self):
        for wpm in (40, 120, 180, 600):
            agent = TypingAgent(wpm)
            count = 0
            for _ in range(600):
                action = agent.next_input(observe(word(), dt=1 / 60))
                self.assertLessEqual(len(action.text), 1)
                count += len(action.text)
            self.assertAlmostEqual(count, wpm * 10 / 12, delta=1.1)

    def test_idle_time_does_not_create_bursts(self):
        agent = TypingAgent()
        for _ in range(60):
            agent.next_input(observe())
        self.assertEqual(agent.next_input(observe(word(), dt=0)).text, "c")
        self.assertEqual(agent.next_input(observe(word(), dt=0)), Action())
        agent.reset()
        self.assertEqual(agent.keystrokes, 0)

    def test_invalid_speed_and_delta(self):
        for wpm in (0, -1, 601, math.nan, math.inf):
            with self.assertRaises(ValueError):
                TypingAgent(wpm)
        agent = TypingAgent()
        for dt in (-1, math.nan, math.inf):
            self.assertEqual(agent.next_input(observe(word(), dt=dt)), Action())

    def test_command_transport(self):
        self.assertEqual(Action("a").command(), "type a")
        self.assertEqual(Action(backspace=True).command(), "backspace")
        self.assertEqual(Action().command(), "idle")


BINARY = Path(os.environ.get("RACEST_TEST_BINARY", ROOT / "build/native/racest"))


@unittest.skipUnless(BINARY.is_file(), "Build the game with make to run integration tests")
class IntegrationTests(unittest.TestCase):
    def run_agent(self, *args):
        return subprocess.run([sys.executable, str(AGENT_DIR / "run.py"), "--binary", str(BINARY),
                               "--simulate", *args], capture_output=True, text=True, timeout=30)

    def result(self, *args):
        process = self.run_agent(*args)
        self.assertEqual(process.returncode, 0, process.stderr)
        return json.loads(process.stdout)

    def test_python_survives_three_ten_minute_runs(self):
        score_file = ROOT / "data/highscore.dat"
        before = score_file.read_bytes() if score_file.exists() else None
        for seed in (1, 42, 2026):
            result = self.result("--seconds", "600", "--wpm", "120", "--seed", str(seed))
            self.assertFalse(result["game_over"], result)
            self.assertEqual(result["seconds"], 600)
            self.assertGreater(result["completed_words"], 300)
            self.assertGreater(result["score"], 5000)
        after = score_file.read_bytes() if score_file.exists() else None
        self.assertEqual(before, after)

    def test_seed_is_reproducible(self):
        args = ("--seconds", "60", "--seed", "42")
        self.assertEqual(self.result(*args), self.result(*args))

    def test_insufficient_speed_loses_normally(self):
        result = self.result("--seconds", "60", "--wpm", "1", "--seed", "42")
        self.assertTrue(result["game_over"])
        self.assertLess(result["seconds"], 60)
        self.assertEqual(result["score"], 0)

    def test_invalid_cli_options(self):
        for args in (("--wpm", "0"), ("--wpm", "nan"), ("--seconds", "0"),
                     ("--seed", "-1"), ("--seed", "4294967296")):
            self.assertNotEqual(self.run_agent(*args).returncode, 0)

    def test_disconnect_and_invalid_command_fail_cleanly(self):
        for command in ("", "award points\n"):
            result = subprocess.run([str(BINARY), "--agent-stdio", "--simulate", "--seconds", "1"],
                                    input=command, text=True, capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 1)
            self.assertIn("observation", result.stdout)


if __name__ == "__main__":
    unittest.main()
