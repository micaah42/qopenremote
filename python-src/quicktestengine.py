"""Python client for the C++ ``QuickTestEngine`` exposed over qopenremote.

Wraps :class:`QOpenRemoteWebSocket` and mirrors the API of
``qopenremote/src/quicktestengine.h`` so QML/Quick applications can be
driven and inspected remotely from Python.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from qopenremote_ws import QOpenRemoteWebSocket

#: Default key under which QuickTestEngine registers itself
#: (see ``_registry.registerValue("quickTestEngine", this)`` in quicktestengine.cpp).
DEFAULT_KEY = "quickTestEngine"


@dataclass
class PathPart:
    """A single step in a path used to locate a QML object.

    Mirrors the C++ ``PathPart`` struct in quicktestengine.h. Combine multiple
    fields for more specific matching, balancing specificity against the
    stability of the QML structure.
    """

    id: str = ""
    typeName: str = ""
    objectName: str = ""
    index: int = -1
    propertyName: str = ""

    def to_dict(self) -> dict[str, Any]:
        """Serialize to the JSON shape expected by the C++ side."""
        return {
            "id": self.id,
            "typeName": self.typeName,
            "objectName": self.objectName,
            "index": self.index,
            "propertyName": self.propertyName,
        }


#: A path is a sequence of PathParts, resolved starting from a top-level window.
Path = list[PathPart]


def _path_to_json(path: Path) -> list[dict[str, Any]]:
    return [part.to_dict() for part in path]


@dataclass
class RecordingFrame:
    """A single recorded interaction, mirroring the C++ ``RecordingFrame`` gadget."""

    Unknown = "Unknown"
    Press = "Press"
    Release = "Release"

    time: str = ""
    action: str = Unknown
    path: Path = field(default_factory=list)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> RecordingFrame:
        return cls(
            time=data.get("time", ""),
            action=data.get("action", cls.Unknown),
            path=[PathPart(**part) for part in data.get("path", [])],
        )


@dataclass
class Recording:
    """A recorded session, mirroring the C++ ``Recording`` gadget."""

    start: str = ""
    end: str = ""
    frames: list[RecordingFrame] = field(default_factory=list)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Recording:
        return cls(
            start=data.get("start", ""),
            end=data.get("end", ""),
            frames=[
                RecordingFrame.from_dict(frame) for frame in data.get("frames", [])
            ],
        )


class QuickTestEngine:
    """Remote proxy for a C++ ``QuickTestEngine`` instance.

    Uses a :class:`QOpenRemoteWebSocket` connection to call methods and
    get/set properties registered under ``key`` (``"quickTestEngine"`` by
    default, matching ``ObjectRegistry2::registerValue("quickTestEngine", this)``).
    """

    def __init__(self, socket: QOpenRemoteWebSocket, key: str = DEFAULT_KEY) -> None:
        self._socket = socket
        self._key = key

    def _property_key(self, name: str) -> str:
        return f"{self._key}.{name}"

    @property
    def event_logging(self) -> bool:
        return bool(self._socket.get(self._property_key("eventLogging")))

    @event_logging.setter
    def event_logging(self, value: bool) -> None:
        self._socket.set(self._property_key("eventLogging"), value)

    @property
    def enabled(self) -> bool:
        return bool(self._socket.get(self._property_key("enabled")))

    @enabled.setter
    def enabled(self, value: bool) -> None:
        self._socket.set(self._property_key("enabled"), value)

    @property
    def address(self) -> str:
        return str(self._socket.get(self._property_key("address")))

    @address.setter
    def address(self, value: str) -> None:
        self._socket.set(self._property_key("address"), value)

    @property
    def port(self) -> int:
        return int(self._socket.get(self._property_key("port")))

    @port.setter
    def port(self, value: int) -> None:
        self._socket.set(self._property_key("port"), value)

    def recording(self) -> Recording:
        """Return the current recording (``QuickTestEngine::recording()``)."""
        return Recording.from_dict(self._socket.get(self._property_key("recording")))

    def save_recording(self, filename: str = "") -> bool:
        """Save the current recording to a JSON file (``QuickTestEngine::saveRecording()``)."""
        return bool(self._call("saveRecording", filename))

    def find(self, path: Path) -> Any:
        """Find a QML object matching ``path`` (``QuickTestEngine::find()``)."""
        return self._call("find", _path_to_json(path))

    def findAwait(self, path: Path, timeout: int = 1000) -> Any:
        """Find or await a QML object matching ``path`` (``QuickTestEngine::findAwait()``)."""
        return self._call("findAwait", _path_to_json(path), timeout)

    def click(self, path: Path, rel_x: float = 0.5, rel_y: float = 0.5) -> bool:
        """Simulate a mouse click on the item at ``path`` (``QuickTestEngine::clickPosition()``)."""
        return bool(self._call("clickPosition", _path_to_json(path), rel_x, rel_y))

    def mouse_press(self, path: Path) -> bool:
        """Simulate a mouse press on the item at ``path`` (``QuickTestEngine::mousePress()``)."""
        return bool(self._call("mousePress", _path_to_json(path)))

    def mouse_release(self, path: Path) -> bool:
        """Simulate a mouse release on the item at ``path`` (``QuickTestEngine::mouseRelease()``)."""
        return bool(self._call("mouseRelease", _path_to_json(path)))

    def _call(self, method: str, *arguments: Any) -> Any:
        return self._socket.call(self._property_key(method), *arguments)
