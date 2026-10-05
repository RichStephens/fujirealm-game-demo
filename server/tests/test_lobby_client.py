import argparse
import json
import threading
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from unittest.mock import patch

from server.hybrid_server import FujiRealmHybridServer, main, parse_lobby_client
from server.lobby_client import LobbyConfig, LobbyPublisher


class _LobbyHandler(BaseHTTPRequestHandler):
    responses = {}
    requests = []

    def _send(self, code):
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b"{}")

    def do_GET(self):
        self.__class__.requests.append(("GET", self.path, None))
        self._send(self.__class__.responses.get(("GET", self.path), 200))

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length) if length else b""
        payload = json.loads(body.decode("utf-8")) if body else None
        self.__class__.requests.append(("POST", self.path, payload))
        self._send(self.__class__.responses.get(("POST", self.path), 200))

    def do_DELETE(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length) if length else b""
        payload = json.loads(body.decode("utf-8")) if body else None
        self.__class__.requests.append(("DELETE", self.path, payload))
        self._send(self.__class__.responses.get(("DELETE", self.path), 200))

    def log_message(self, format, *args):
        return


class LobbyClientTest(unittest.TestCase):
    def setUp(self):
        _LobbyHandler.requests = []
        _LobbyHandler.responses = {}
        self.server = ThreadingHTTPServer(("127.0.0.1", 0), _LobbyHandler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.errors = []
        self.config = LobbyConfig(
            base_url=f"http://127.0.0.1:{self.server.server_port}",
            game="FujiRealm Demo",
            creator_id=0x3022,
            app_id=0x02,
            server="The Realm",
            region="us",
            server_url="tcp://fujinet.online:9010",
            max_players=32,
            clients=(
                ("atari", "TNFS://tnfs.fujinet.online/ATARI/netgames/fujirealm.atr"),
                ("intv", "https://example.com/fujirealm.rom"),
                ("coco", "https://example.com/FUJIRLM3.dsk"),
            ),
            timeout=0.2,
        )
        self.publisher = LobbyPublisher(self.config, self.errors.append)

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=2)

    def test_payload_contains_expected_fields(self):
        payload = self.config.payload(4, "online")
        self.assertEqual(payload["game"], "FujiRealm Demo")
        self.assertEqual(payload["appkey"], 0x02)
        self.assertEqual(payload["serverurl"], "tcp://fujinet.online:9010")
        self.assertEqual(payload["curplayers"], 4)
        self.assertEqual(payload["clients"], [
            {"platform": platform, "url": url} for platform, url in self.config.clients
        ])

    def test_publish_posts_server_payload(self):
        self.assertTrue(self.publisher.publish(2, "online"))
        method, path, payload = _LobbyHandler.requests[-1]
        self.assertEqual((method, path), ("POST", "/server"))
        self.assertEqual(payload["appkey"], 0x02)
        self.assertEqual(payload["curplayers"], 2)
        self.assertEqual(payload["status"], "online")
        self.assertEqual(payload["clients"], self.config.payload(2, "online")["clients"])

    def test_delete_posts_serverurl(self):
        self.assertTrue(self.publisher.delete())
        method, path, payload = _LobbyHandler.requests[-1]
        self.assertEqual((method, path), ("DELETE", "/server"))
        self.assertEqual(payload, {"serverurl": "tcp://fujinet.online:9010"})

    def test_probe_version_hits_health_endpoint(self):
        self.assertTrue(self.publisher.probe_version())
        self.assertEqual(_LobbyHandler.requests[-1], ("GET", "/version", None))

    def test_publish_http_error_is_nonfatal(self):
        _LobbyHandler.responses[("POST", "/server")] = 500
        self.assertFalse(self.publisher.publish(1, "online"))
        self.assertTrue(self.errors)

    def test_publish_http_error_logs_response_body(self):
        _LobbyHandler.responses[("POST", "/server")] = 400
        self.assertFalse(self.publisher.publish(1, "online"))
        self.assertTrue(self.errors)
        self.assertIn("body=", self.errors[-1])

    def test_lobby_client_argument(self):
        self.assertEqual(parse_lobby_client("coco=https://example.com/download?file=a=b"),
                         ("coco", "https://example.com/download?file=a=b"))
        for value in ("coco", "=https://example.com", "coco=", "  =  "):
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                parse_lobby_client(value)

    def test_cli_passes_repeated_clients_to_server(self):
        argv = ["hybrid_server", "--lobby-enabled",
                "--lobby-client", "intv=https://example.com/intv.rom",
                "--lobby-client", "coco=https://example.com/coco.dsk"]
        with patch("sys.argv", argv), patch("server.hybrid_server.FujiRealmHybridServer") as server:
            self.assertEqual(main(), 0)
        self.assertEqual(server.call_args.kwargs["lobby_clients"], (
            ("intv", "https://example.com/intv.rom"),
            ("coco", "https://example.com/coco.dsk"),
        ))
        server.return_value.serve.assert_called_once_with()

    def test_server_merges_default_and_configured_clients(self):
        server = FujiRealmHybridServer("127.0.0.1", 0, lobby_enabled=True,
            lobby_clients=(("intv", "https://example.com/intv.rom"),
                           ("coco", "https://example.com/coco.dsk"),
                           ("atari", "https://example.com/atari.atr")))
        self.assertEqual(server.lobby.config.payload(0, "online")["clients"], [
            {"platform": "atari", "url": "https://example.com/atari.atr"},
            {"platform": "intv", "url": "https://example.com/intv.rom"},
            {"platform": "coco", "url": "https://example.com/coco.dsk"},
        ])


if __name__ == "__main__":
    unittest.main()
