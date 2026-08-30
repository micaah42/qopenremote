"""Synchronous client for qopenremote's WebSocket JSON protocol.

Requires the ``websocket-client`` package.
"""

import json
import threading
from collections.abc import Callable
from importlib import import_module
from typing import Any

NotificationHandler = Callable[[str, Any], None]


class QOpenRemoteCallError(RuntimeError):
    """A remote method call completed without a return value."""

    pass


class QOpenRemoteWebSocket:
    """Connect to an ObjectRegistry2 exposed by ``WebSocketServer``.

    The server protocol does not attach request IDs to responses, so requests
    are serialized and a single instance should not issue requests concurrently.
    """

    def __init__(
        self,
        url: str = "ws://127.0.0.1:21120",
        timeout: float | None = 5.0,
        on_notification: NotificationHandler | None = None,
    ) -> None:
        self.url = url
        self.timeout = timeout
        self.on_notification = on_notification
        self._socket: Any | None = None
        self._lock = threading.Lock()

    @property
    def connected(self) -> bool:
        """Whether a WebSocket connection is currently open."""
        return self._socket is not None and self._socket.connected

    def connect(self) -> None:
        """Open the WebSocket connection."""
        if self.connected:
            return

        websocket = import_module("websocket")
        self._socket = websocket.create_connection(self.url, timeout=self.timeout)

    def disconnect(self) -> None:
        """Close the WebSocket connection, if one is open."""
        if self._socket is not None:
            self._socket.close()
            self._socket = None

    def get(self, key: str) -> Any:
        """Return the JSON-compatible value registered under ``key``."""
        return self._request_value({"type": "get", "key": key}, key)

    def set(self, key: str, value: Any) -> None:
        """Set ``key`` to a JSON-compatible value.

        qopenremote does not send an acknowledgement for set requests.
        """
        self._send({"type": "set", "key": key, "value": value})

    def call(self, key: str, *arguments: Any) -> Any:
        """Invoke the registered method ``key`` and return its result."""
        return self._request_value(
            {"type": "call", "key": key, "args": list(arguments)}, key
        )

    def subscribe(self, key: str) -> Any:
        """Subscribe to ``key`` changes and return its initial value.

        Future notifications are delivered to ``on_notification``.
        """
        return self._request_value(
            {"type": "subscribe", "key": key}, key, response_type="notify"
        )

    def _send(self, message: dict[str, Any]) -> None:
        if not self.connected:
            raise ConnectionError(
                "not connected; call connect() before sending requests"
            )
        self._socket.send(json.dumps(message))

    def _request_value(
        self, message: dict[str, Any], key: str, response_type: str = "return"
    ) -> Any:
        with self._lock:
            self._send(message)
            while True:
                response = self._receive_message()
                if response.get("type") == "notify":
                    self._handle_notification(response)
                    if response_type == "notify" and response.get("key") == key:
                        return response.get("value")
                    continue
                if response.get("type") == "error" and response.get("key") == key:
                    raise QOpenRemoteCallError(
                        str(response.get("error", "remote call failed"))
                    )
                if response.get("type") == response_type and response.get("key") == key:
                    return response.get("value")

    def _receive_message(self) -> dict[str, Any]:
        if not self.connected:
            raise ConnectionError("connection closed while waiting for a response")
        message = self._socket.recv()
        if not message:
            self._socket = None
            raise ConnectionError("connection closed while waiting for a response")
        try:
            decoded = json.loads(message)
        except json.JSONDecodeError as error:
            raise ValueError("received invalid JSON from qopenremote") from error
        if not isinstance(decoded, dict):
            raise ValueError("received a non-object JSON message from qopenremote")
        return decoded

    def _handle_notification(self, message: dict[str, Any]) -> None:
        if self.on_notification is not None:
            self.on_notification(message["key"], message.get("value"))

    def __enter__(self) -> "QOpenRemoteWebSocket":
        self.connect()
        return self

    def __exit__(self, *_: object) -> None:
        self.disconnect()
