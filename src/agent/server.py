"""Local browser-game host; all typing decisions run in policy.py."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
from threading import Lock
import time

from policy import TypingAgent


class AgentServer(ThreadingHTTPServer):
    def __init__(self, address, public, default_wpm=120):
        super().__init__(address, partial(AgentHandler, directory=str(public)))
        self.default_wpm = default_wpm
        self.sessions = {}
        self.agent_lock = Lock()


class AgentHandler(SimpleHTTPRequestHandler):
    def log_message(self, *_args):
        pass

    def send_json(self, value):
        data = json.dumps(value).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/api/agent/config":
            self.send_json({"wpm": self.server.default_wpm})
        else:
            super().do_GET()

    def do_POST(self):
        if self.path != "/api/agent/step":
            self.send_error(404)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 65536:
                raise ValueError("Invalid request size")
            payload = json.loads(self.rfile.read(length))
            session = payload["session"]
            if not isinstance(session, str) or not 1 <= len(session) <= 64:
                raise ValueError("Invalid session")
            wpm = float(payload.get("wpm", self.server.default_wpm))
            epoch = payload["epoch"]
            observation = payload["observation"]
            with self.server.agent_lock:
                now = time.monotonic()
                self.server.sessions = {key: value for key, value in self.server.sessions.items()
                                        if now - value[2] < 300}
                entry = self.server.sessions.get(session)
                if entry is None or entry[1] != epoch:
                    agent = TypingAgent(wpm)
                else:
                    agent = entry[0]
                    if agent.wpm != wpm:
                        agent.set_wpm(wpm)
                action = agent.next_input(observation)
                self.server.sessions[session] = (agent, epoch, now)
            self.send_json(action.as_dict())
        except (ValueError, KeyError, TypeError, OverflowError) as error:
            self.send_error(400, str(error))


def serve(public: Path, port=8000, wpm=120):
    if not all((public / name).is_file() for name in ("index.html", "index.js", "index.wasm", "index.data")):
        raise RuntimeError("Browser build is missing. Run make web first.")
    with AgentServer(("127.0.0.1", port), public, wpm) as server:
        print(f"RACEST + Python agent: http://localhost:{server.server_port}", flush=True)
        server.serve_forever()
