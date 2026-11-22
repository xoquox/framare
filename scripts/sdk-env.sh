#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
environment=$(nix-build --no-out-link "$project_dir/nix/sdk-fhs.nix")
exec "$environment/bin/framare-sdk-env" "$@"
