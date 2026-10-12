"""Download and cache the online vocabulary used by RACEST."""
from dataclasses import dataclass
from datetime import datetime, timezone
import json
from pathlib import Path
import tempfile
import time
from urllib.error import URLError
from urllib.request import Request, urlopen

DATASET_URL = (
    "https://raw.githubusercontent.com/first20hours/google-10000-english/"
    "master/google-10000-english-no-swears.txt"
)
DATASET_PAGE = "https://github.com/first20hours/google-10000-english"
MAX_BYTES = 2 * 1024 * 1024
MAX_AGE = 24 * 60 * 60


@dataclass(frozen=True)
class WordSource:
    path: Path
    words: tuple
    source: str
    warning: str = ""


def clean_words(text):
    """Keep unique, typeable English words, preserving dataset order."""
    result = []
    seen = set()
    for line in text.splitlines():
        word = line.strip().lower()
        if 3 <= len(word) <= 12 and word.isascii() and word.isalpha() and word not in seen:
            result.append(word)
            seen.add(word)
    return tuple(result)


def read_cache(path):
    try:
        if path.stat().st_size > MAX_BYTES:
            return ()
        text = path.read_text(encoding="utf-8")
        words = clean_words(text)
        # Native play reads this file directly, so reject partly corrupted files
        # instead of silently filtering only the Python view of the cache.
        canonical = "\n".join(words) + "\n"
        return words if len(words) >= 100 and text == canonical else ()
    except (OSError, UnicodeError):
        return ()


def atomic_write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                         prefix=".words-", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(text)
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def resolve_words(root, refresh=False):
    """Use a fresh cache, download a dataset, or fall back without losing data."""
    root = Path(root)
    cache = root / "data/words.online.txt"
    cached = read_cache(cache)
    if cached and not refresh and 0 <= time.time() - cache.stat().st_mtime < MAX_AGE:
        return WordSource(cache, cached, "cache")
    try:
        request = Request(DATASET_URL, headers={"User-Agent": "RACEST/1.0"})
        with urlopen(request, timeout=8) as response:
            raw = response.read(MAX_BYTES + 1)
        if len(raw) > MAX_BYTES:
            raise ValueError("Dataset response exceeded the size limit")
        words = clean_words(raw.decode("utf-8"))
        if len(words) < 100:
            raise ValueError("Dataset did not contain enough usable words")
        # The game only sees the file after a fully validated download is saved.
        atomic_write(cache, "\n".join(words) + "\n")
        try:
            atomic_write(root / "data/words.online.json", json.dumps({
                "dataset": DATASET_PAGE, "url": DATASET_URL, "word_count": len(words),
                "downloaded_at": datetime.now(timezone.utc).isoformat(),
                "minimum_length": 3, "maximum_length": 12,
            }, indent=2) + "\n")
        except OSError:
            pass  # Metadata is informational; the validated word cache is usable.
        return WordSource(cache, words, "download")
    except (OSError, URLError, ValueError, UnicodeError) as error:
        warning = f"Online dataset unavailable ({error})"
        if cached:
            return WordSource(cache, cached, "cache", warning + "; using cached words.")
        bundled = root / "assets/words.txt"
        try:
            words = clean_words(bundled.read_text(encoding="utf-8"))
        except (OSError, UnicodeError):
            words = ()
        return WordSource(bundled, words, "bundled", warning + "; using bundled words.")


def describe(source):
    message = source.warning or f"Word dataset: {len(source.words)} words ({source.source})."
    return message
