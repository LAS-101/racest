"""Optional end-to-end smoke test: build the web game first; needs Playwright."""
from pathlib import Path
import re
import sys
from threading import Thread

from playwright.sync_api import sync_playwright


def main():
    root = Path(__file__).resolve().parents[2]
    sys.path.insert(0, str(root / "src/agent"))
    from server import AgentServer
    public = root / "build/web/public"
    if not (public / "index.wasm").exists():
        raise RuntimeError("Build the browser game with make web first")
    server = AgentServer(("127.0.0.1", 0), public)
    Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(
                headless=True,
                args=["--no-sandbox", "--use-angle=swiftshader", "--enable-unsafe-swiftshader"],
            )
            try:
                page = browser.new_page(viewport={"width": 1280, "height": 1050})
                errors = []
                page.on("pageerror", lambda error: errors.append(str(error)))
                page.route("**/favicon.ico", lambda route: route.fulfill(status=204))
                page.add_init_script("localStorage.setItem('racest.highscore', '123')")
                page.goto(f"http://127.0.0.1:{server.server_port}")
                page.wait_for_function("!document.getElementById('start-agent').disabled")
                page.locator("#start-agent").click()
                page.wait_for_function("document.getElementById('status').textContent.includes('Python agent playing at 120 WPM')")
                page.wait_for_function(
                    "Number(document.getElementById('status').textContent.match(/score (\\d+)/)?.[1]) > 0",
                    timeout=15000,
                )
                status = page.locator("#status").inner_text()
                assert "completed" in status, status
                print(status)
                page.locator("#agent-speed").select_option("180")
                page.wait_for_function("document.getElementById('status').textContent.includes('180 WPM')")

                # Moving focus away pauses gameplay, including the agent.
                page.locator("#stop-agent").focus()
                page.wait_for_function("document.getElementById('status').textContent.includes('Paused')")
                page.locator("#canvas").focus()
                page.wait_for_function("document.getElementById('status').textContent.includes('Python agent playing')")
                page.locator("#canvas").screenshot(path=str(root / "build/agent-browser.png"))

                page.locator("#stop-agent").click()
                page.wait_for_function("document.getElementById('status').textContent.startsWith('Playing')")
                before = int(re.search(r"score (\d+)", page.locator("#status").inner_text()).group(1))
                page.wait_for_timeout(250)
                after = int(re.search(r"score (\d+)", page.locator("#status").inner_text()).group(1))
                assert before == after, "Agent kept typing after Take control"
                # raylib polls function-key state once per frame, so hold the
                # key across frames like a physical keypress.
                page.locator("#canvas").press("F2", delay=100)
                page.wait_for_function("document.getElementById('status').textContent.includes('Python agent playing')")
                page.locator("#canvas").press("F2", delay=100)
                page.wait_for_function("document.getElementById('status').textContent.startsWith('Playing')")
                assert page.evaluate("localStorage.getItem('racest.highscore')") == "123"
                assert not errors, errors
                print("Browser checks passed: live typing, speed changes, pause/resume, takeover, F2, and score isolation.")
            finally:
                browser.close()
    finally:
        server.shutdown()
        server.server_close()


if __name__ == "__main__":
    main()
