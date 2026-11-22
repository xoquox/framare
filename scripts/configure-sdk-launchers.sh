#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
sdk_root="$HOME/SailfishOS"
if [[ ! -x "$sdk_root/bin/sfdk" ]]; then
  printf 'Install the SDK before configuring its launchers.\n' >&2
  exit 1
fi
# Keep the SDK runtime alive across Nix garbage collection. SDK launchers then
# work independently of the source checkout and do not need nix-build on start.
nix-build "$project_dir/nix/sdk-fhs.nix" -o "$sdk_root/framare-nix-env"
cat > "$sdk_root/bin/framare-sdk-launch" <<'SH'
#!/bin/sh
set -eu
case "${1:-}" in
  qtcreator)
    program="$HOME/SailfishOS/bin/qtcreator"
    export RESOURCE_NAME=SailfishSDK-qtcreator
    ;;
  maintenance) program="$HOME/SailfishOS/SDKMaintenanceTool" ;;
  *) printf 'Expected qtcreator or maintenance\n' >&2; exit 2 ;;
esac
shift
exec "$HOME/SailfishOS/framare-nix-env/bin/framare-sdk-env" -c \
  'exec "$@"' framare-sdk "$program" "$@"
SH
chmod 0755 "$sdk_root/bin/framare-sdk-launch"
python3 - "$sdk_root" <<'PY'
from pathlib import Path
import shutil
import sys

sdk = Path(sys.argv[1])
applications = Path.home() / '.local/share/applications'
backup = sdk / 'framare-launchers-backup'
backup.mkdir(exist_ok=True)
launcher = sdk / 'bin/framare-sdk-launch'
for filename, args in (
    ('SailfishSDK-qtcreator.desktop', 'qtcreator'),
    ('SailfishSDK-MaintenanceTool.desktop', 'maintenance -name SailfishSDK-MaintenanceTool'),
):
    desktop = applications / filename
    old = desktop.read_text()
    if not (backup / filename).exists():
        shutil.copy2(desktop, backup / filename)
    # This setup uses the fixed SDK location in the current user's home.
    # Reject Desktop Entry field codes/metacharacters in that path.
    if any(c in str(launcher) for c in '%"\\\n\r`$'):
        sys.exit('Unsupported launcher path; desktop files left unchanged')
    lines = old.splitlines(True)
    if sum(line.startswith('Exec=') for line in lines) != 1:
        sys.exit(f'Unexpected desktop file structure: {desktop}')
    desktop.write_text(''.join(
        f'Exec="{launcher}" {args}\n' if line.startswith('Exec=') else line
        for line in lines
    ))
    print('Configured:', desktop)
PY
