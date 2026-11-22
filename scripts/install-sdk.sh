#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd -- "$project_dir"
if [[ -x "$HOME/SailfishOS/bin/sfdk" ]]; then
  printf 'SDK already exists at %s; inspect it before changing components.\n' "$HOME/SailfishOS"
  exit 0
fi
if [[ -e "$HOME/SailfishOS" ]]; then
  printf 'Incomplete SDK directory exists; inspect %s before retrying.\n' "$HOME/SailfishOS" >&2
  exit 1
fi
docker info >/dev/null
mkdir -p .local/logs
log_file="$project_dir/.local/logs/sdk-install.log"
printf 'Installing SDK as normal user. Log: %s\n' "$log_file"
QT_QPA_PLATFORM=minimal ./.local/downloads/SailfishSDK-3.13.5-linux64-online.run --verbose non-interactive=1 accept-licenses=1 build-engine-type=docker > "$log_file" 2>&1
# The installer can exit 0 even when its prerequisite check rejects Docker.
if [[ ! -x "$HOME/SailfishOS/bin/sfdk" ]]; then
  tail -n 35 "$log_file" >&2
  printf 'SDK installation did not produce sfdk. Inspect the log.\n' >&2
  exit 1
fi
printf 'SDK installed. Verify targets and build engine before using it.\n'
