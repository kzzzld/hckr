import os
from pathlib import Path
import subprocess
import tempfile
import unittest


APP = Path(__file__).resolve().parents[1] / 'build/hckr'
DEFAULTS = ('[hackernews]\nbase_url = https://news.ycombinator.com\n\n'
            '[ui]\ninitial_tab = new\n')


class ConfigTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.env = dict(os.environ, HOME=str(self.root),
                        XDG_CONFIG_HOME=str(self.root / 'nested/config'))
        self.env.pop('HCKR_HN_URL', None)
        self.path = Path(self.env['XDG_CONFIG_HOME']) / 'hckr/config.ini'

    def run_app(self):
        return subprocess.run([str(APP)], env=self.env, capture_output=True,
                              text=True, timeout=5)

    def assert_loaded(self):
        result = self.run_app()
        self.assertEqual(result.returncode, 1)
        self.assertIn('an interactive terminal is required', result.stderr)

    def test_creates_defaults_without_persisting_environment(self):
        self.env['HCKR_HN_URL'] = 'https://example.com'
        self.assert_loaded()
        self.assertEqual(self.path.read_text(), DEFAULTS)
        self.assertEqual(self.path.stat().st_mode & 0o777, 0o600)
        self.assertEqual(self.path.parent.stat().st_mode & 0o777, 0o700)
        self.assert_loaded()
        self.assertEqual(self.path.read_text(), DEFAULTS)

    def test_preserves_existing_configuration(self):
        self.path.parent.mkdir(parents=True)
        content = '[ui]\ninitial_tab = jobs\n'
        self.path.write_text(content)
        self.assert_loaded()
        self.assertEqual(self.path.read_text(), content)

    def test_preserves_invalid_configuration(self):
        self.path.parent.mkdir(parents=True)
        content = '[ui]\ninitial_tab = invalid\n'
        self.path.write_text(content)
        result = self.run_app()
        self.assertIn(str(self.path) + ':2:', result.stderr)
        self.assertEqual(self.path.read_text(), content)

    def test_optional_colors(self):
        self.path.parent.mkdir(parents=True)
        for setting in ('foreground', 'background'):
            for color in ('default', 'black', 'red', 'green', 'yellow', 'blue',
                          'magenta', 'cyan', 'white', '0', '15', '255'):
                with self.subTest(setting=setting, color=color):
                    self.path.write_text(f'[ui]\n{setting} = {color}\n')
                    self.assert_loaded()
            for color in ('', '-1', '256', 'purple', '12x', '99999999999999999999999'):
                with self.subTest(setting=setting, color=color):
                    self.path.write_text(f'[ui]\n{setting} = {color}\n')
                    result = self.run_app()
                    self.assertIn(str(self.path) + ':2:', result.stderr)
                    self.assertIn('color must be', result.stderr)

    def test_home_fallback(self):
        self.env.pop('XDG_CONFIG_HOME')
        self.assert_loaded()
        self.assertEqual((self.root / '.config/hckr/config.ini').read_text(), DEFAULTS)

    def test_creation_failure(self):
        (self.root / 'nested').write_text('blocked')
        result = self.run_app()
        self.assertEqual(result.returncode, 1)
        self.assertIn(str(self.path), result.stderr)
        self.assertNotIn('interactive terminal', result.stderr)


if __name__ == '__main__':
    unittest.main()
