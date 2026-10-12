#!/usr/bin/env python3
"""Play RACEST using an online word dataset, with offline fallback."""
import argparse
from pathlib import Path
import subprocess
import sys

from dataset import describe, resolve_words

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--offline", action="store_true", help="Use the original bundled word bank")
    parser.add_argument("--refresh-words", action="store_true", help="Refresh the internet dataset now")
    parser.add_argument("--download-only", action="store_true", help="Download/cache words without opening the game")
    parser.add_argument("--binary", type=Path, default=ROOT / "build/native/racest")
    parser.add_argument("--seed", type=int)
    options = parser.parse_args()
    if options.offline and options.refresh_words:
        parser.error("--offline cannot be combined with --refresh-words")
    if options.seed is not None and not 0 <= options.seed <= 4294967295:
        parser.error("Seed must be an unsigned 32-bit integer")
    try:
        source = None
        words_path = ROOT / "assets/words.txt"
        if not options.offline:
            source = resolve_words(ROOT, options.refresh_words)
            words_path = source.path
            print(describe(source), file=sys.stderr)
        if options.download_only:
            return int(source is not None and bool(source.warning))
        if not options.binary.is_file():
            raise RuntimeError("Game executable missing. Build it with make first.")
        command = [str(options.binary.resolve()), "--words-file", str(words_path)]
        if options.seed is not None:
            command.extend(["--seed", str(options.seed)])
        return subprocess.call(command, cwd=ROOT)
    except KeyboardInterrupt:
        return 130
    except (OSError, RuntimeError) as error:
        print(f"RACEST: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
