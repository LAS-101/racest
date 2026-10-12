# Internet word source

RACEST downloads its online vocabulary from the
[Google 10,000 English words dataset](https://github.com/first20hours/google-10000-english),
using `google-10000-english-no-swears.txt`. The source is a common-word list
intended for typing training. Words are selected randomly by the game using
its existing difficulty rules, rather than requested individually from a web API.

## Play

From the repository root:

```bash
make run                 # Human play with online words
make run-local           # Original local vocabulary
make agent-online        # Autonomous play with online words
make serve-web           # Browser game, agent, and online vocabulary
make refresh-words       # Refresh the dataset without playing
```

The native game must be built first when running the launcher directly:

```bash
python3 src/words/run.py
python3 src/words/run.py --refresh-words
python3 src/words/run.py --offline
python3 src/words/run.py --download-only --refresh-words
```

Use `--binary` for another native build, and `--seed` for repeatable selection
from an unchanged dataset. Agent launch options:

```bash
python3 src/agent/run.py --online-words --wpm 180
python3 src/agent/run.py --simulate --online-words --seconds 600 --seed 42
python3 src/agent/run.py --web --online-words
```

## Download and cache

`dataset.py` keeps unique lowercase ASCII words of 3–12 letters. This matches
the keyboard controls and keeps the vocabulary usable at every difficulty.
A download must contain at least 100 valid words before it can replace a cache.
Responses are limited to 2 MiB with an eight-second network timeout.

Downloaded words are stored in `data/words.online.txt`, and their source URL,
word count, and download time are recorded in `data/words.online.json`.
Both files are local runtime data, ignored by Git. A valid cache is reused for
24 hours; `--refresh-words` forces a download. The original `assets/words.txt`
is preserved. A failed download uses the previous valid cache, or the bundled
list if no cache exists. A download-only refresh returns an error status if
the refresh failed, while ordinary play continues with its fallback vocabulary.

Downloads and validation happen before starting the game or browser server;
no internet calls are made while words are moving. In the browser, the server
supplies the chosen vocabulary before play starts and the page displays its
source. A static host uses its bundled vocabulary. Agent decisions still run
in Python and work with either source.

## Tests

```bash
make test
python3 -m unittest discover -s tests/words -v
```

Tests check normalization, deduplication, caching, forced refresh, malformed or
oversized responses, write failures, and offline fallback without accessing the
network. The browser smoke test also loads a large vocabulary and verifies
that spawned words come from it:

```bash
python3 tests/agent/browser_smoke.py
```
