import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

APP = Path(__file__).resolve().parents[1] / 'build/browser_test'


@unittest.skipIf(sys.platform == 'win32', 'POSIX launcher stub tests')
class BrowserTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.command = self.root / ('open' if sys.platform == 'darwin' else 'xdg-open')
        self.log = self.root / 'arguments'
        self.command.write_text('#!/bin/sh\nprintf "%s\\n" "$#" "$1" > "$BROWSER_TEST_LOG"\n')
        self.command.chmod(0o700)
        self.env = dict(os.environ, PATH=str(self.root), BROWSER_TEST_LOG=str(self.log))

    def run_browser(self, url):
        return subprocess.run([str(APP), url], env=self.env, text=True,
                              capture_output=True, timeout=10)

    def test_passes_url_as_one_literal_argument(self):
        url = 'https://example.com/article?q=$(touch%20should-not-exist)&a=1'
        result = self.run_browser(url)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.log.read_text(), f'1\n{url}\n')

    def test_rejects_invalid_and_non_web_links(self):
        for url in ('', 'javascript:alert(1)', 'file:///tmp/test', '-arg',
                    'https://example.com/\nunsafe'):
            with self.subTest(url=url):
                self.assertEqual(self.run_browser(url).returncode, 1)
        self.assertFalse(self.log.exists())

    def test_missing_launcher(self):
        self.command.unlink()
        result = self.run_browser('https://example.com')
        self.assertEqual(result.returncode, 1)
        self.assertIn('could not run', result.stderr)

    def test_launcher_failure(self):
        self.command.write_text('#!/bin/sh\nexit 1\n')
        result = self.run_browser('https://example.com')
        self.assertEqual(result.returncode, 1)
        self.assertIn('could not be opened', result.stderr)


if __name__ == '__main__':
    unittest.main()
