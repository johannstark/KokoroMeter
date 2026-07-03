"""Tests for the KokoroMeter CLI."""

from unittest.mock import patch

from typer.testing import CliRunner

from cli.main import app

runner = CliRunner()


def test_status_not_running():
    """Test the status command when daemon is not running."""
    with patch("cli.main.get_pid", return_value=None):
        result = runner.invoke(app, ["status"])
        assert result.exit_code == 0
        assert "NOT running" in result.stdout


def test_status_running():
    """Test the status command when daemon is running."""
    with patch("cli.main.get_pid", return_value=1234):
        result = runner.invoke(app, ["status"])
        assert result.exit_code == 0
        assert "running (PID: 1234)" in result.stdout


def test_stop_not_running():
    """Test the stop command when daemon is not running."""
    with patch("cli.main.get_pid", return_value=None):
        result = runner.invoke(app, ["stop"])
        assert result.exit_code == 0
        assert "not running" in result.stdout


@patch("cli.main.os.kill")
@patch("pathlib.Path.unlink")
def test_stop_running(mock_unlink, mock_kill):
    """Test the stop command when daemon is running."""
    with patch("cli.main.get_pid", return_value=1234):
        result = runner.invoke(app, ["stop"])
        assert result.exit_code == 0
        assert "Daemon stopped" in result.stdout
        mock_kill.assert_called_once()
        mock_unlink.assert_called_once()


@patch("cli.main.subprocess.Popen")
@patch("cli.main.PID_FILE")
def test_start_already_running(mock_pid_file, mock_popen):
    """Test the start command when it's already running."""
    with patch("cli.main.get_pid", return_value=1234):
        result = runner.invoke(app, ["start"])
        assert result.exit_code == 0
        assert "already running" in result.stdout
        mock_popen.assert_not_called()
