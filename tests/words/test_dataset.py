import io
import json
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
from urllib.error import URLError

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/words"))
import dataset


WORDS = ["cat" + chr(97 + i // 26) + chr(97 + i % 26) for i in range(100)]
PAYLOAD = ("\n".join(WORDS) + "\n").encode()


class DatasetTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        (self.root / "assets").mkdir()
        (self.root / "assets/words.txt").write_text("space\nstar\nplanet\n", encoding="utf-8")
        self.cache = self.root / "data/words.online.txt"

    def tearDown(self):
        self.directory.cleanup()

    def seed_cache(self, stale=False):
        dataset.atomic_write(self.cache, PAYLOAD.decode())
        if stale:
            old = time.time() - dataset.MAX_AGE - 10
            os.utime(self.cache, (old, old))

    def test_filters_normalizes_and_deduplicates(self):
        self.assertEqual(dataset.clean_words("CAT\ncat\na\ntwo words\nhello!\ncafé\nabc123\nPLANET\n"),
                         ("cat", "planet"))

    @patch("dataset.urlopen")
    def test_download_is_cached_with_provenance(self, fetch):
        fetch.return_value = io.BytesIO(PAYLOAD)
        source = dataset.resolve_words(self.root)
        self.assertEqual(source.source, "download")
        self.assertEqual(source.words, tuple(WORDS))
        self.assertEqual(self.cache.read_bytes(), PAYLOAD)
        metadata = json.loads((self.root / "data/words.online.json").read_text())
        self.assertEqual(metadata["url"], dataset.DATASET_URL)
        self.assertEqual(metadata["word_count"], 100)
        self.assertEqual(fetch.call_args.kwargs["timeout"], 8)

    @patch("dataset.urlopen")
    def test_fresh_cache_needs_no_network(self, fetch):
        self.seed_cache()
        source = dataset.resolve_words(self.root)
        self.assertEqual(source.source, "cache")
        fetch.assert_not_called()

    @patch("dataset.urlopen", side_effect=URLError("offline"))
    def test_stale_cache_survives_network_failure(self, fetch):
        self.seed_cache(stale=True)
        before = self.cache.read_bytes()
        source = dataset.resolve_words(self.root)
        self.assertEqual(source.source, "cache")
        self.assertIn("cached", source.warning)
        self.assertEqual(self.cache.read_bytes(), before)
        fetch.assert_called_once()

    @patch("dataset.urlopen", side_effect=URLError("offline"))
    def test_first_run_offline_uses_bundled_words(self, _fetch):
        source = dataset.resolve_words(self.root)
        self.assertEqual(source.source, "bundled")
        self.assertEqual(source.words, ("space", "star", "planet"))
        self.assertFalse(self.cache.exists())

    @patch("dataset.urlopen")
    def test_force_refresh_replaces_a_fresh_cache(self, fetch):
        self.seed_cache()
        replacement = ("\n".join(word.replace("cat", "dog") for word in WORDS) + "\n").encode()
        fetch.return_value = io.BytesIO(replacement)
        source = dataset.resolve_words(self.root, refresh=True)
        self.assertEqual(source.source, "download")
        self.assertEqual(self.cache.read_bytes(), replacement)
        fetch.assert_called_once()

    @patch("dataset.urlopen")
    def test_invalid_download_preserves_cached_words(self, fetch):
        self.seed_cache()
        for payload in (b"<html>service unavailable</html>", b"\xff", b"x" * (dataset.MAX_BYTES + 1)):
            fetch.return_value = io.BytesIO(payload)
            source = dataset.resolve_words(self.root, refresh=True)
            self.assertEqual(source.source, "cache")
            self.assertEqual(self.cache.read_bytes(), PAYLOAD)

    @patch("dataset.urlopen", side_effect=URLError("offline"))
    def test_bad_cache_does_not_become_game_words(self, _fetch):
        dataset.atomic_write(self.cache, "<html>error</html>")
        self.assertEqual(dataset.resolve_words(self.root).source, "bundled")
        dataset.atomic_write(self.cache, PAYLOAD.decode() + "<html>bad</html>\n")
        self.assertEqual(dataset.resolve_words(self.root).source, "bundled")

    @patch("dataset.atomic_write", side_effect=OSError("read only"))
    @patch("dataset.urlopen")
    def test_cache_write_failure_falls_back(self, fetch, _write):
        fetch.return_value = io.BytesIO(PAYLOAD)
        self.assertEqual(dataset.resolve_words(self.root).source, "bundled")
        self.assertFalse(self.cache.exists())


if __name__ == "__main__":
    unittest.main()
