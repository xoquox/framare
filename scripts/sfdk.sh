#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
# Keep the caller's working directory and pass every argument separately.
exec bash "$project_dir/scripts/sdk-env.sh" -c \
  'exec "${FRAMARE_SDK_ROOT:-$HOME/SailfishOS}/bin/sfdk" "$@"' framare-sfdk "$@"
