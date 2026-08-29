"""Integration tests for the qopenremote WebSocket client."""

import os
import socket
import subprocess
import sys
import time
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parents[1] / "python-src"))

from qopenremote_ws import QOpenRemoteWebSocket


def _available_port() -> int:
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


@pytest.fixture
def client() -> QOpenRemoteWebSocket:
    """Run the dedicated qopenremote server and yield a connected client."""
    server_path = os.environ.get("QOPENREMOTE_TEST_SERVER")
    if server_path is None:
        pytest.fail("QOPENREMOTE_TEST_SERVER must name the qopenremote test server")

    port = _available_port()
    server = subprocess.Popen(
        [server_path, "--port", str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    websocket_client = QOpenRemoteWebSocket(url=f"ws://127.0.0.1:{port}", timeout=0.1)

    deadline = time.monotonic() + 3
    while True:
        try:
            websocket_client.connect()
            break
        except OSError:
            if server.poll() is not None:
                out, _ = server.communicate()
                pytest.fail(
                    f"qopenremote test server exited before accepting connections:\n{out}"
                )
            if time.monotonic() >= deadline:
                server.terminate()
                out, _ = server.communicate()
                pytest.fail(
                    f"qopenremote test server did not accept connections within deadline:\n{out}"
                )
            time.sleep(0.05)

    try:
        yield websocket_client
    finally:
        websocket_client.disconnect()
        server.terminate()
        try:
            out, _ = server.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            out, _ = server.communicate()
        if out:
            print(f"\n--- qopenremote test server logs ---\n{out}")


def test_get(client: QOpenRemoteWebSocket) -> None:
    """Retrieve the test server's initial property value."""
    assert client.connected
    assert client.get("testServer.value") == 0


def test_set(client: QOpenRemoteWebSocket) -> None:
    """Set a property and retrieve its new value."""
    client.set("testServer.value", 42)
    assert client.get("testServer.value") == 42


def test_call(client: QOpenRemoteWebSocket) -> None:
    """Invoke a registered method and return its result."""
    assert client.call("testServer.add", 5) == 5
