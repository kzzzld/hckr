import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('package_release', ROOT / 'scripts/package_release.py')
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class ReleaseTest(unittest.TestCase):
    def test_versions(self):
        self.assertEqual(release.release_version('v0.1.0'), '0.1.0')
        for tag in ('0.1.0', 'v1', 'v01.2.3', 'v1.2.3-rc1', '../v1.2.3'):
            with self.subTest(tag=tag):
                with self.assertRaises(ValueError):
                    release.release_version(tag)

    def test_archive_and_checksum(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            def git(*args):
                return subprocess.check_output(['git', '-c', 'commit.gpgsign=false', '-c', 'tag.gpgsign=false',
                                                '-c', 'core.hooksPath=/dev/null', '-C', str(root), *args])
            git('init', '-b', 'main')
            (root / 'README.md').write_text('Release fixture\n')
            git('add', 'README.md')
            git('-c', 'commit.gpgsign=false', '-c', 'core.hooksPath=/dev/null',
                '-c', 'user.name=Release Test', '-c', 'user.email=test@example.com',
                'commit', '-m', 'Fixture')
            git('tag', 'v0.1.0')
            import os
            previous = Path.cwd()
            try:
                os.chdir(root)
                archive = release.package_release('v0.1.0', root / 'dist')
                original = archive.read_bytes()
                with tarfile.open(archive) as source:
                    self.assertEqual(source.getnames(), ['hckr-0.1.0', 'hckr-0.1.0/README.md'])
                expected = hashlib.sha256(original).hexdigest()
                self.assertEqual((root / 'dist/SHA256SUMS').read_text(),
                                 f'{expected}  hckr-0.1.0.tar.gz\n')
                release.package_release('v0.1.0', root / 'dist')
                self.assertEqual(archive.read_bytes(), original)
            finally:
                os.chdir(previous)


if __name__ == '__main__':
    unittest.main()
