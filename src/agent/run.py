#!/usr/bin/env python3
"""Launch RACEST and let the Python agent play its real game loop."""
import argparse
import json
import math
from pathlib import Path
import subprocess
import sys

from policy import TypingAgent

ROOT = Path(__file__).resolve().parents[2]


def bounded_number(minimum, maximum):
    def parse(value):
        result = float(value)
        if not math.isfinite(result) or not minimum <= result <= maximum:
            raise argparse.ArgumentTypeError(f"Must be between {minimum} and {maximum}")
        return result
    return parse


def parse_seed(value):
    try:
        result = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("Seed must be an unsigned 32-bit integer") from error
    if not 0 <= result <= 4294967295:
        raise argparse.ArgumentTypeError("Seed must be an unsigned 32-bit integer")
    return result


def stop_process(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def run_native(options):
    binary = options.binary.resolve()
    if not binary.is_file():
        raise RuntimeError(f"Game executable missing: {binary}. Build it with make first.")
    command = [str(binary), "--agent-stdio", "--agent-wpm", str(options.wpm)]
    if options.seed is not None:
        command.extend(["--seed", str(options.seed)])
    if options.simulate:
        command.extend(["--simulate", "--seconds", str(options.seconds)])
    agent = TypingAgent(options.wpm)
    last_round = None
    last_enabled = False
    result = None
    with subprocess.Popen(command, cwd=ROOT, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                          text=True, encoding="utf-8", bufsize=1) as process:
        try:
            for line in process.stdout:
                message = json.loads(line)
                if message["kind"] == "result":
                    result = message
                    continue
                if message["kind"] != "observation":
                    raise RuntimeError("Unexpected game protocol message")
                enabled = message["enabled"]
                if message["round"] != last_round or (enabled and not last_enabled):
                    agent.reset()
                last_round, last_enabled = message["round"], enabled
                action = agent.next_input(message)
                process.stdin.write(action.command() + "\n")
                process.stdin.flush()
            return_code = process.wait()
            if return_code:
                raise RuntimeError(f"RACEST exited with status {return_code}")
            if options.simulate and result is None:
                raise RuntimeError("Simulation ended without a result")
            if result is not None:
                print(json.dumps(result), flush=True)
            return 0
        finally:
            stop_process(process)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Python typing agent for RACEST")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--simulate", action="store_true", help="Evaluate without a window")
    mode.add_argument("--web", action="store_true", help="Serve the browser game and Python agent")
    parser.add_argument("--wpm", "--agent-wpm", type=bounded_number(1, 600), default=120)
    parser.add_argument("--seconds", type=bounded_number(0.000001, 86400), default=300,
                        help="Maximum simulation duration (default: 300)")
    parser.add_argument("--seed", type=parse_seed, help="Reproducible game seed")
    parser.add_argument("--binary", type=Path, default=ROOT / "build/native/racest", help="Native game executable")
    parser.add_argument("--port", type=int, default=8000, help="Local browser server port")
    options = parser.parse_args(argv)
    if not 1 <= options.port <= 65535:
        parser.error("Port must be between 1 and 65535")
    try:
        if options.web:
            from server import serve
            serve(ROOT / "build/web/public", options.port, options.wpm)
            return 0
        return run_native(options)
    except KeyboardInterrupt:
        print("Python agent stopped.", file=sys.stderr)
        return 130
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(f"Python agent: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
