"""Tests for the KokoroMeter CLI."""

import tempfile
from pathlib import Path
from unittest.mock import patch, MagicMock

from typer.testing import CliRunner

from cli.main import app, get_gcloud_credentials

runner = CliRunner()


def test_get_gcloud_credentials_missing(tmp_path):
    """Test get_gcloud_credentials when no credentials exist."""
    with patch("cli.main.Path.home", return_value=tmp_path):
        try:
            get_gcloud_credentials()
            assert False, "Should raise typer.Exit"
        except Exception:
            pass


@patch("cli.main.subprocess.run")
@patch("cli.main.get_gcloud_credentials")
@patch("cli.main.find_esp32_port")
@patch("cli.main.perform_provisioning")
def test_setup_command(mock_provision, mock_find, mock_gcloud, mock_run):
    """Test the interactive setup command."""
    mock_gcloud.return_value = ("mock_id", "mock_secret", "mock_token")
    mock_find.return_value = "/dev/cu.usbserial-0001"
    
    # 1. Hardware Profile (1 - Hosyond)
    # 2. Confirm Flash (y)
    user_inputs = "1\ny\n"
    
    result = runner.invoke(app, ["setup"], input=user_inputs)
    
    assert result.exit_code == 0
    assert "Firmware flashed successfully" in result.stdout
    mock_run.assert_called_once()
    mock_find.assert_called_once()
    mock_provision.assert_called_once_with("/dev/cu.usbserial-0001", "mock_id", "mock_secret", "mock_token")
