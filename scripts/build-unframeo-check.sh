#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source_dir="$project_dir/.local/upstream/unframeo"
output_dir="$project_dir/.local/transport-check"
expected_revision=$(cat "$project_dir/nix/unframeo-revision")
if [[ $(git -C "$source_dir" rev-parse HEAD) != "$expected_revision" ]] ||
   [[ -n $(git -C "$source_dir" status --porcelain --untracked-files=normal) ]]; then
  printf 'Expected a clean unframeo checkout at %s\n' "$expected_revision" >&2
  exit 1
fi
export GOTOOLCHAIN=local CGO_ENABLED=0 GOOS=linux
mkdir -p -- "$output_dir"
cd -- "$source_dir"
go version
go mod verify
GOARCH=amd64 go build -mod=readonly -trimpath -buildvcs=false -o "$output_dir/unframeo-amd64" ./cmd/unframeo
GOARCH=arm64 go build -mod=readonly -trimpath -buildvcs=false -o "$output_dir/unframeo-aarch64" ./cmd/unframeo
GOARCH=arm GOARM=7 go build -mod=readonly -trimpath -buildvcs=false -o "$output_dir/unframeo-armv7hl" ./cmd/unframeo
"$output_dir/unframeo-amd64" help > "$output_dir/help.txt"
cp LICENSE NOTICE "$output_dir/"
printf 'Source: https://github.com/borje/unframeo\nCommit: %s\nLocal build checks only; no Sailfish runtime validation.\n' \
  "$expected_revision" > "$output_dir/SOURCES.txt"
file "$output_dir"/unframeo-*
sha256sum "$output_dir"/unframeo-*
