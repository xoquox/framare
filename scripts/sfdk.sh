#!/usr/bin/env bash
set -euo pipefail
# An optional executable wrapper can handle host-specific runtime requirements.
sdk_program=${FRAMARE_SFDK:-"${FRAMARE_SDK_ROOT:-$HOME/SailfishOS}/bin/sfdk"}
exec "$sdk_program" "$@"
