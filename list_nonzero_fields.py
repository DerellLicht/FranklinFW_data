"""
list_nonzero_fields.py - one-off survey: which FranklinWH API fields ever hold data?

Asks the cloud API (through franklinwh-cli, with --all-fields) for a week, a month, a
year and your lifetime totals, plus one day of 5-minute power samples, and reports every
column that has at least one non-zero value, with its largest value and where it was seen.
Columns that were always zero or empty are listed by name at the end.

Usage:
    python list_nonzero_fields.py
    python list_nonzero_fields.py --date 2026-11-09 --day 2026-11-20

Takes 5 CLI calls (5 logins).  Stops at the first failure, because retrying a rejected
login can lock the account.  The report is printed and saved as nonzero_fields.txt
beside this script.
"""

from __future__ import annotations

import argparse
import sys
from datetime import date, timedelta

from franklin_common import SCRIPT_DIR, CliError, ConfigError, load_config, run_energy_csv

LABEL_COLUMNS = {"date", "time", "run_status_label"}   # not measurements


def field_dictionary() -> dict:
    """
    Column name -> 'description [unit]' from the franklinwh-cloud field dictionary,
    or {} if that module can't be imported (the report still works, just without text).
    """
    try:
        from franklinwh_cloud.const.energy_fields import ELECTRIC_FIELDS, POWER_FIELDS
    except Exception:
        return {}
    info = {}
    for f in tuple(ELECTRIC_FIELDS) + tuple(POWER_FIELDS):
        text = f.description or ""
        if f.unit:
            text += f" [{f.unit}]"
        info[f.column] = text
    return info


def as_number(text: str):
    """The float value of `text`, or None if it is blank or not numeric."""
    try:
        return float(text)
    except (TypeError, ValueError):
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description="List API fields that have any non-zero data.")
    ap.add_argument("--config", help="path to franklinwh.ini (default: beside this script)")
    ap.add_argument("--date", help="a date inside the week/month/year to scan, YYYY-MM-DD (default: last week)")
    ap.add_argument("--day", help="day for the 5-minute power scan, YYYY-MM-DD (default: yesterday)")
    args = ap.parse_args()

    today = date.today()
    scan_date = date.fromisoformat(args.date) if args.date else today - timedelta(days=7)
    scan_date -= timedelta(days=scan_date.weekday())          # week queries need a Monday
    power_day = date.fromisoformat(args.day) if args.day else today - timedelta(days=1)

    scans = [("week", "week", scan_date, None),
             ("month", "month", scan_date, None),
             ("year", "year", scan_date, None),
             ("lifetime", "lifetime", today, None),
             ("day-5min", "day", power_day, "5min")]

    try:
        cfg = load_config(args.config)
        nonzero = {}      # column -> list of (scan name, max abs value, label where seen)
        seen = {}         # column -> True if it was ever non-blank in any scan
        order = []        # columns in first-seen order

        for name, period, when, interval in scans:
            print(f"scanning {name} ({when}) ...", flush=True)
            columns, rows = run_energy_csv(cfg, period, when.isoformat(), interval=interval)
            for col in columns:
                if col in LABEL_COLUMNS:
                    continue
                if col not in seen:
                    seen[col] = False
                    order.append(col)
                best, best_label = None, ""
                for row in rows:
                    value = as_number(row.get(col, ""))
                    if value is None:
                        continue
                    seen[col] = True
                    if value != 0 and (best is None or abs(value) > abs(best)):
                        best, best_label = value, row.get("date") or row.get("time") or ""
                if best is not None:
                    nonzero.setdefault(col, []).append((name, best, best_label))
    except (ConfigError, CliError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    info = field_dictionary()
    out = ["Fields with non-zero data", "=" * 25, ""]
    for col in order:
        if col not in nonzero:
            continue
        out.append(col + ("   - " + info[col] if info.get(col) else "   - (not in the field dictionary)"))
        for name, best, label in nonzero[col]:
            out.append(f"      {name:<9} max {best:g}   (at {label})")
    out += ["", "Always zero", "=" * 11, ""]
    out.append(", ".join(c for c in order if c not in nonzero and seen[c]) or "(none)")
    out += ["", "Never any value (blank in every scan)", "=" * 37, ""]
    out.append(", ".join(c for c in order if not seen[c]) or "(none)")

    text = "\n".join(out) + "\n"
    print()
    print(text)
    (SCRIPT_DIR / "nonzero_fields.txt").write_text(text, encoding="utf-8")
    print(f"Saved: {SCRIPT_DIR / 'nonzero_fields.txt'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
