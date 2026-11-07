from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
import unittest

APP = Path(__file__).resolve().parents[1] / 'build/feed_test'
STORIES = (b'<table><tr class="athing" id="1"><td><span class="titleline">'
           b'<a href="item?id=1">Test &amp; news</a></span></td></tr>'
           b'<tr><td class="subtext"><span class="score">986 points</span>'
           b'<a href="item?id=1">1 hour ago</a> | '
           b'<a href="item?id=1">174&nbsp;comments</a></td></tr></table>')
COMMENTS = b'<tr class="athing" id="42"><td><div class="commtext c00">A <i>test</i> comment &#x1b; safe<p>Next paragraph</p></div></td></tr>'


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        bodies = {
            '/news': STORIES, '/newest': STORIES, '/front': STORIES, '/ask': STORIES,
            '/show': STORIES, '/jobs': STORIES, '/newcomments': COMMENTS,
            '/external': b'<span class="titleline"><a href="https://example.com/a?a=1&amp;b=2">External</a></span>',
            '/root': b'<span class="titleline"><a href="/item?id=2">Root</a></span>',
            '/protocol': b'<span class="titleline"><a href="//example.com/story">Protocol</a></span>',
            '/missing': b'<span class="titleline"><a>Missing</a></span>',
            '/discuss': STORIES.replace(b'174&nbsp;comments', b'discuss'),
            '/one': STORIES.replace(b'986 points', b'1 point').replace(b'174&nbsp;comments', b'1 comment'),
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
        for path in ('/news', '/newest', '/front', '/ask', '/show', '/jobs', '/redirect'):
            with self.subTest(path=path):
                result = self.fetch(path)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn('Test & news', result.stdout)
                self.assertIn('points=986 comments=174', result.stdout)
                self.assertIn(self.url + 'item?id=1', result.stdout)

    def test_comments_and_control_characters(self):
        result = self.fetch('/newcomments')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('A test comment', result.stdout)
        self.assertNotIn('\x1b', result.stdout)
        self.assertIn('safe Next paragraph', result.stdout)
        self.assertIn(self.url + 'item?id=42', result.stdout)

    def test_link_resolution(self):
        for path, url in (('/external', 'https://example.com/a?a=1&b=2'),
                          ('/root', self.url + 'item?id=2'),
                          ('/protocol', 'http://example.com/story'),
                          ('/missing', 'no URL')):
            with self.subTest(path=path):
                result = self.fetch(path)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(url, result.stdout)

    def test_metrics(self):
        for path, expected in (('/discuss', 'points=986 comments=0'),
                               ('/one', 'points=1 comments=1'),
                               ('/missing', 'points=-1 comments=-1'),
                               ('/newcomments', 'points=-1 comments=-1')):
            with self.subTest(path=path):
                result = self.fetch(path)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(expected, result.stdout)

    def test_failures(self):
        for path, status in (('/unavailable', 2), ('/empty', 3),
                             ('/invalid', 3), ('/oversized', 2)):
            with self.subTest(path=path):
                result = self.fetch(path)
                self.assertEqual(result.returncode, status, result.stderr)
                self.assertTrue(result.stderr.strip())


if __name__ == '__main__':
    unittest.main()
