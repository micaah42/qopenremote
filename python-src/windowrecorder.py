"""Record named X11 windows with ffmpeg."""

from __future__ import annotations

import logging
import os
import re
import subprocess
from os import PathLike

_WINDOW_LINE = re.compile(r'^\s*(0x[0-9a-fA-F]+) "([^"]*)":')


def find_window_id(window_name: str) -> str:
    """Return the X11 window ID whose title exactly matches ``window_name``.

    Raises ``LookupError`` when no matching named window exists or when the
    title is ambiguous.
    """
    result = subprocess.run(
        ["xwininfo", "-tree", "-root"],
        check=True,
        capture_output=True,
        text=True,
    )
    matches = [
        match.group(1)
        for line in result.stdout.splitlines()
        if (match := _WINDOW_LINE.match(line)) is not None
        and match.group(2) == window_name
    ]

    if not matches:
        raise LookupError(f"no X11 window named {window_name!r} was found")
    if len(matches) > 1:
        logging.warning("found multiple X11 windows named %r", window_name)
    return matches[0]


def create_ffmpeg_subprocess(
    window_id: str | int,
    output_path: str | PathLike[str],
    *,
    display: str | None = None,
    framerate: int = 30,
) -> subprocess.Popen[str]:
    """Start recording an X11 window and return its ffmpeg process.

    Send ``q`` to the returned process's standard input to finalize the file.
    """
    if framerate <= 0:
        raise ValueError("framerate must be greater than zero")

    return subprocess.Popen(
        [
            "ffmpeg",
            "-f",
            "x11grab",
            "-framerate",
            str(framerate),
            "-draw_mouse",
            "0",
            "-window_id",
            str(window_id),
            "-i",
            display or os.environ.get("DISPLAY", ":0"),
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            "-preset",
            "slow",
            "-crf",
            "16",
            "-profile:v",
            "high",
            "-movflags",
            "+faststart",
            os.fspath(output_path),
        ],
        stdin=subprocess.PIPE,
        text=True,
    )
