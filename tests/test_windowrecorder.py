"""Unit tests for X11 window recording helpers."""

import subprocess
import sys
from pathlib import Path
from unittest.mock import Mock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parents[1] / "python-src"))

from windowrecorder import create_ffmpeg_subprocess, find_window_id


@patch("windowrecorder.subprocess.run")
def test_find_window_id_returns_exact_title_match(mock_run: Mock) -> None:
    mock_run.return_value = subprocess.CompletedProcess(
        ["xwininfo"],
        0,
        stdout=(
            '     0x1200001 "Clock": ("clockd" "clockd")\n'
            '     0x1200002 "Clock Settings": ("clockd" "clockd")\n'
        ),
    )

    assert find_window_id("Clock") == "0x1200001"
    mock_run.assert_called_once_with(
        ["xwininfo", "-tree", "-root"],
        check=True,
        capture_output=True,
        text=True,
    )


@patch("windowrecorder.subprocess.run")
def test_find_window_id_uses_first_match_and_rejects_missing_titles(
    mock_run: Mock, caplog: pytest.LogCaptureFixture
) -> None:
    mock_run.return_value = subprocess.CompletedProcess(
        ["xwininfo"],
        0,
        stdout='  0x1 "Clock": ()\n  0x2 "Clock": ()\n',
    )

    assert find_window_id("Clock") == "0x1"
    assert "multiple X11 windows" in caplog.text
    with pytest.raises(LookupError, match="no X11 window"):
        find_window_id("Missing")


@patch("windowrecorder.subprocess.Popen")
def test_create_ffmpeg_subprocess_uses_x11grab(mock_popen: Mock) -> None:
    process = Mock()
    mock_popen.return_value = process

    assert create_ffmpeg_subprocess("0x1200001", "clock.mp4", display=":1") is process
    mock_popen.assert_called_once_with(
        [
            "ffmpeg",
            "-f",
            "x11grab",
            "-framerate",
            "30",
            "-draw_mouse",
            "0",
            "-window_id",
            "0x1200001",
            "-i",
            ":1",
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
            "clock.mp4",
        ],
        stdin=subprocess.PIPE,
        text=True,
    )


def test_create_ffmpeg_subprocess_rejects_non_positive_framerate() -> None:
    with pytest.raises(ValueError, match="greater than zero"):
        create_ffmpeg_subprocess("0x1", "clock.mp4", framerate=0)
