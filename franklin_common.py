"""
Shared helpers for the FranklinWH data scripts (weekly_export.py, list_nonzero_fields.py).

Both scripts get their data by running `franklinwh-cli energy ...` (from the
franklinwh-cloud package) as a child process, so the login, token handling and
API details stay in that library.  Settings live in one franklinwh.ini file
beside these scripts:

    [FranklinWH]
    email    = you@example.com
    password = your-FRANKLINWH-app-password      (not the Enlighten one)
    gateway  = 10070052S00X25150016

    [Export]
    output_dir = data                            (relative to this folder)

franklinwh.ini holds a password in plain text: keep it OUT of git.
"""

from __future__ import annotations

import configparser
import csv
import io
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

# Folder holding the scripts and (by default) franklinwh.ini.  Everything relative
# is resolved against this, never against the current directory, so the scripts
# behave the same under Task Scheduler as they do from a command prompt.
SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_INI = SCRIPT_DIR / "franklinwh.ini"

# One bad-password login counts toward FranklinWH's account lockout, so nothing in
# these scripts ever retries a failed CLI call.
CLI_TIMEOUT_SECONDS = 180


class ConfigError(Exception):
    """The ini file is missing, unreadable, or lacks a required setting."""


class CliError(Exception):
    """franklinwh-cli failed (login rejected, network trouble, no data...)."""


@dataclass
class Config:
    ini_path: Path
    email: str
    password: str
    gateway: str
    output_dir: Path


def load_config(ini_path: Path | str | None = None) -> Config:
    """
    Read franklinwh.ini and return the settings.

    ini_path : explicit path, or None for franklinwh.ini beside the scripts.
    Raises ConfigError if the file or any of email/password/gateway is missing.
    output_dir defaults to "data" and, if relative, is taken relative to SCRIPT_DIR.
    """
    path = Path(ini_path) if ini_path else DEFAULT_INI
    if not path.is_file():
        raise ConfigError(f"Config file not found: {path}\n"
                          f"Copy franklinwh.ini.example to franklinwh.ini and fill it in.")

    parser = configparser.ConfigParser()
    parser.read(path, encoding="utf-8")

    values = {}
    for key in ("email", "password", "gateway"):
        value = parser.get("FranklinWH", key, fallback="").strip()
        if not value:
            raise ConfigError(f"{path}: [FranklinWH] {key} is missing or empty.")
        values[key] = value

    out = Path(parser.get("Export", "output_dir", fallback="data").strip() or "data")
    if not out.is_absolute():
        out = SCRIPT_DIR / out

    return Config(path.resolve(), values["email"], values["password"], values["gateway"], out)


def run_energy_csv(config: Config, period: str, date_iso: str, *,
                   interval: str | None = None) -> tuple[list[str], list[dict]]:
    """
    Run `franklinwh-cli energy --all-fields --format csv` and parse the result.

    period   : day | week | month | year | ytd | lifetime
    date_iso : YYYY-MM-DD (a week must be given as its Monday)
    interval : "5min" for the 5-minute power series (period must be "day")

    Returns (column names, rows) where each row is {column: text value}.
    Empty text means the API sent no value ("no data" is not zero).
    Raises CliError on any failure; never retries (see CLI_TIMEOUT_SECONDS note).

    Notes on the child process:
      * run as `python -m franklinwh_cloud.cli` so PATH / Scripts folders don't matter
      * UTF-8 is forced because the CLI prints non-ASCII symbols, which crash a
        piped Windows console using the default code page
      * the CLI prints some warnings on stdout, so the CSV is located by its header
        line rather than assuming it is the whole output
    """
    cmd = [sys.executable, "-m", "franklinwh_cloud.cli",
           "--config", str(config.ini_path), "--no-color",
           "energy", "--period", period, "--date", date_iso,
           "--format", "csv", "--all-fields"]
    if interval:
        cmd += ["--interval", interval]

    env = dict(os.environ, PYTHONIOENCODING="utf-8", PYTHONUTF8="1", NO_COLOR="1")
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8",
                              errors="replace", env=env, timeout=CLI_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired as exc:
        raise CliError(f"franklinwh-cli timed out after {CLI_TIMEOUT_SECONDS} s") from exc
    except OSError as exc:
        raise CliError(f"Could not start franklinwh-cli: {exc}") from exc

    if proc.returncode != 0:
        detail = (proc.stdout + proc.stderr).strip() or "(no output)"
        raise CliError(f"franklinwh-cli failed (exit {proc.returncode}):\n{detail}")

    return parse_cli_csv(proc.stdout)


def parse_cli_csv(text: str) -> tuple[list[str], list[dict]]:
    """
    Pull the CSV table out of the CLI's stdout.

    The table starts at the first line whose first field is "date" or "time"
    (energy and 5-minute power output respectively); anything before it, such as the
    "No gateway specified" warning, is ignored.
    Raises CliError if no header line is found or the table has no rows.
    """
    lines = text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if line.split(",", 1)[0].strip() in ("date", "time") and "," in line:
            start = i
            break
    if start is None:
        raise CliError("No CSV table in franklinwh-cli output:\n" + text.strip())

    reader = csv.reader(io.StringIO("\n".join(lines[start:])))
    columns = next(reader)
    rows = [dict(zip(columns, rec)) for rec in reader if rec]
    if not rows:
        raise CliError("franklinwh-cli returned a table with no rows.")
    return columns, rows
