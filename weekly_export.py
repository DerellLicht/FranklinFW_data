"""
weekly_export.py - build the weekly FranklinWH summary file from the cloud API.

Writes data-YYYY-MM-DD-week.csv (YYYY-MM-DD = the week's Monday) in the same layout
the FranklinWH iOS app exports, so it can replace the manual Monday download:

    Date,Home (kWh),Solar (kWh),aPower(s) Charge (kWh),aPower(s) Discharge (kWh),
    Grid Import (kWh),Grid Export (kWh),Generator (kWh),V2L (kWh)

Usage:
    python weekly_export.py                      # the last complete Mon-Sun week
    python weekly_export.py --week-start 2026-11-09
    python weekly_export.py --force              # overwrite an existing file

Settings (login, gateway, output folder) come from franklinwh.ini; see franklin_common.py.
Exit status: 0 = file written (or already present), 1 = failure (nothing written).
"""

from __future__ import annotations

import argparse
import os
import sys
from datetime import date, timedelta
from decimal import Decimal, InvalidOperation, ROUND_HALF_UP
from pathlib import Path

from franklin_common import CliError, ConfigError, load_config, run_energy_csv

# App column header -> CLI column name, in the app's column order (after "Date").
APP_COLUMNS = [
    ("Home (kWh)",                "home_kwh"),
    ("Solar (kWh)",               "solar_kwh"),
    ("aPower(s) Charge (kWh)",    "battery_charge_kwh"),
    ("aPower(s) Discharge (kWh)", "battery_discharge_kwh"),
    ("Grid Import (kWh)",         "grid_import_kwh"),
    ("Grid Export (kWh)",         "grid_export_kwh"),
    ("Generator (kWh)",           "generator_kwh"),
    ("V2L (kWh)",                 "v2l_kwh"),
]

# The API has never reported V2L for the system this was written for, and the CLI may
# omit or blank it; for this column only, "missing" is written as 0.0.  Every other
# column must have a real value or the whole export is refused.
OPTIONAL_COLUMNS = {"v2l_kwh"}

MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]


def last_complete_week_start(today: date) -> date:
    """Monday of the most recent Mon-Sun week that has fully ended before `today`."""
    this_monday = today - timedelta(days=today.weekday())
    return this_monday - timedelta(days=7)


def app_date(d: date) -> str:
    """Date as the app writes it, e.g. 'Nov 9, 2026' (month names fixed, not locale-dependent)."""
    return f"{MONTHS[d.month - 1]} {d.day}, {d.year}"


def one_decimal(text: str) -> str:
    """
    Round an API kWh value (e.g. '8.695') to one decimal place, half-up, as text.
    A result that rounds to zero is always written '0.0' (never '-0.0').
    Raises ValueError if `text` is not a number.
    """
    try:
        value = Decimal(text).quantize(Decimal("0.1"), rounding=ROUND_HALF_UP)
    except InvalidOperation as exc:
        raise ValueError(f"not a number: {text!r}") from exc
    if value == 0:
        return "0.0"
    return str(value)


def build_app_rows(week_start: date, rows: list[dict]) -> list[list[str]]:
    """
    Convert CLI rows for one week into app-layout rows (without the header).

    Refuses (ValueError) anything that is not a clean Monday-Sunday week: wrong row
    count, dates out of order, or a missing/blank/non-numeric value in a required
    column.  A blank in the wrong-gateway case looks exactly like this, so it must
    never be turned into zeros.
    """
    if len(rows) != 7:
        raise ValueError(f"expected 7 daily rows, got {len(rows)}")

    out = []
    for i, row in enumerate(rows):
        expected = week_start + timedelta(days=i)
        if row.get("date", "").strip() != expected.isoformat():
            raise ValueError(f"row {i + 1}: expected date {expected}, got {row.get('date')!r}")

        cells = [app_date(expected)]
        for header, key in APP_COLUMNS:
            raw = (row.get(key) or "").strip()
            if raw == "":
                if key in OPTIONAL_COLUMNS:
                    cells.append("0.0")
                    continue
                raise ValueError(f"{expected}: no value for '{header}' (wrong gateway, or the week is incomplete?)")
            try:
                cells.append(one_decimal(raw))
            except ValueError as exc:
                raise ValueError(f"{expected}: '{header}': {exc}") from exc
        out.append(cells)
    return out


def render_app_csv(app_rows: list[list[str]]) -> str:
    """
    Render the file text exactly as the app does: CRLF line ends, no trailing newline,
    and the date quoted because it contains a comma.
    """
    header = "Date," + ",".join(h for h, _ in APP_COLUMNS)
    lines = [header]
    for cells in app_rows:
        lines.append('"' + cells[0] + '",' + ",".join(cells[1:]))
    return "\r\n".join(lines)


def write_atomic(path: Path, text: str) -> None:
    """Write `text` to `path` via a temp file + rename, so a crash never leaves a half file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".tmp")
    with open(tmp, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)
    os.replace(tmp, path)


def main() -> int:
    ap = argparse.ArgumentParser(description="Write the weekly FranklinWH summary CSV from the cloud API.")
    ap.add_argument("--config", help="path to franklinwh.ini (default: beside this script)")
    ap.add_argument("--week-start", help="the week's Monday, YYYY-MM-DD (default: last complete week)")
    ap.add_argument("--output-dir", help="override output_dir from the ini file")
    ap.add_argument("--force", action="store_true", help="overwrite the output file if it exists")
    args = ap.parse_args()

    try:
        cfg = load_config(args.config)

        if args.week_start:
            week_start = date.fromisoformat(args.week_start)
            if week_start.weekday() != 0:
                print(f"ERROR: --week-start {week_start} is not a Monday.", file=sys.stderr)
                return 1
        else:
            week_start = last_complete_week_start(date.today())

        out_dir = Path(args.output_dir) if args.output_dir else cfg.output_dir
        out_file = out_dir / f"data-{week_start.isoformat()}-week.csv"
        if out_file.exists() and not args.force:
            print(f"Already exists, nothing to do: {out_file}  (use --force to overwrite)")
            return 0

        _columns, rows = run_energy_csv(cfg, "week", week_start.isoformat())
        app_rows = build_app_rows(week_start, rows)
        write_atomic(out_file, render_app_csv(app_rows))
        print(f"Wrote {out_file}")
        return 0

    except (ConfigError, CliError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
