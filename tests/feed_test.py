from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
import unittest

APP = Path(__file__).resolve().parents[1] / 'build/feed_test'
STORIES = b'<span class="titleline"><a href="item?id=1">Test &amp; news</a></span>'
COMMENTS = b'<div class="commtext c00">A <i>test</i> comment &#x1b; safe<p>Next paragraph</p></div>'


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        bodies = {
            '/newest': STORIES, '/front': STORIES, '/ask': STORIES,
            '/show': STORIES, '/jobs': STORIES, '/newcomments': COMMENTS,
            '/empty': b'', '/invalid': b'<html>unexpected page</html>',
            '/oversized': b'x' * (2 * 1024 * 1024 + 1),
        }
        if self.path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/newest')
            self.end_headers()
            return
        self.send_response(200 if self.path in bodies else 503)
        self.end_headers()
        try:
            self.wfile.write(bodies.get(self.path, b'unavailable'))
        except (BrokenPipeError, ConnectionResetError):
            pass

    def log_message(self, *_):
        pass


class FeedTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.url = f'http://127.0.0.1:{cls.server.server_port}/'

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join()

    def fetch(self, path):
        return subprocess.run([str(APP), self.url, path], text=True,
                              capture_output=True, timeout=15)

    def test_all_tabs_and_entities(self):
        for path in ('/newest', '/front', '/ask', '/show', '/jobs', '/redirect'):
            with self.subTest(path=path):
                result = self.fetch(path)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn('Test & news', result.stdout)

    def test_comments_and_control_characters(self):
        result = self.fetch('/newcomments')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('A test comment', result.stdout)
        self.assertNotIn('\x1b', result.stdout)
        self.assertIn('safe Next paragraph', result.stdout)

    def test_failures(self):
        for path, status in (('/unavailable', 2), ('/empty', 3),
                             ('/invalid', 3), ('/oversized', 2)):
            with self.subTest(path=path):
                result = self.fetch(path)
                self.assertEqual(result.returncode, status, result.stderr)
                self.assertTrue(result.stderr.strip())


if __name__ == '__main__':
    unittest.main()
