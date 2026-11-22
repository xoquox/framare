#!/usr/bin/env python3
"""Preview or install the Framare module in the existing NixOS configuration."""
import argparse
import datetime
import difflib
import getpass
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile


def plan(original, user):
    if not re.fullmatch(r"[a-z_][a-z0-9_-]*\$?", user) or user == "root":
        raise ValueError("Select an existing non-root development user")
    module_import = "    ./framare-development.nix\n"
    if module_import in original:
        if "framareDevelopment = {" not in original:
            raise ValueError("Existing Framare import without settings; inspect manually")
        return original
    if "framareDevelopment" in original or "framare-development.nix" in original:
        raise ValueError("Existing Framare settings have a different layout; inspect manually")
    anchor = "    ./hardware-configuration.nix\n  ];"
    if original.count(anchor) != 1:
        raise ValueError("Unrecognized imports block; configuration left unchanged")
    return original.replace(
        anchor,
        "    ./hardware-configuration.nix\n" + module_import + "  ];\n\n"
        '  framareDevelopment = {\n    enable = true;\n    user = "' + user + '";\n  };',
        1,
    )


def atomic_write(path, content, mode):
    fd, name = tempfile.mkstemp(prefix=".framare-", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as target:
            target.write(content)
            target.flush()
            os.fsync(target.fileno())
            os.fchmod(target.fileno(), mode)
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true", help="Install module and edit /etc/nixos; requires root")
    parser.add_argument("--user", default=os.environ.get("SUDO_USER") or getpass.getuser(),
                        help="Existing development user (default: invoking user)")
    args = parser.parse_args()
    config_path = Path("/etc/nixos/configuration.nix")
    module_path = config_path.parent / "framare-development.nix"
    source_path = Path(__file__).resolve().parents[1] / "nix/nixos-development.nix"
    original = config_path.read_text()
    updated = plan(original, args.user)
    module = source_path.read_text()
    if module_path.exists() and module_path.read_text() != module:
        sys.exit("Existing module differs; inspect it before replacing")
    sys.stdout.writelines(difflib.unified_diff(
        original.splitlines(True), updated.splitlines(True),
        fromfile=str(config_path), tofile=str(config_path) + " (prepared)",
    ))
    if not args.apply:
        print("Preview only. No system files changed.")
        return
    if os.geteuid() != 0:
        sys.exit("--apply requires root. Run with sudo in your local terminal.")
    if config_path.is_symlink() or module_path.is_symlink():
        sys.exit("Refusing to replace a symlink; inspect manually")
    if updated != original:
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
        backup = config_path.with_name("configuration.nix.before-framare-" + stamp)
        shutil.copy2(config_path, backup)
        print("Backup:", backup)
    atomic_write(module_path, module, 0o644)
    if updated != original:
        if config_path.read_text() != original:
            sys.exit("Configuration changed concurrently; edit aborted")
        stat = config_path.stat()
        atomic_write(config_path, updated, stat.st_mode & 0o777)
        os.chown(config_path, stat.st_uid, stat.st_gid)
    print("Configuration installed. Next: sudo nixos-rebuild build && sudo nixos-rebuild switch")


if __name__ == "__main__":
    main()
