#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
architecture=${1:?Usage: bash upstream/build.sh aarch64|armv7hl}
case "$architecture" in
    aarch64) export GOARCH=arm64 ;;
    armv7hl) export GOARCH=arm GOARM=7 ;;
    *) printf 'Unsupported target architecture\n' >&2; exit 2 ;;
esac
revision=$(cat "$root/upstream/unframeo-revision")
source_dir="$root/.local/upstream/unframeo"
if [[ ! -d "$source_dir/.git" ]]; then
    mkdir -p -- "$(dirname -- "$source_dir")"
    git clone --no-checkout https://github.com/borje/unframeo.git "$source_dir"
    git -C "$source_dir" checkout --detach "$revision"
fi
if [[ $(git -C "$source_dir" rev-parse HEAD) != "$revision" ]] ||
   [[ -n $(git -C "$source_dir" status --porcelain) ]]; then
    printf 'Expected a clean unframeo checkout at %s\n' "$revision" >&2; exit 1
fi
export GOTOOLCHAIN=local CGO_ENABLED=0 GOOS=linux
if [[ $(go env GOVERSION) != go1.26.1 ]]; then
    printf 'This build is pinned to Go 1.26.1.\n' >&2; exit 1
fi
destination="$root/.local/unframeo/$architecture"
licenses="$root/.local/unframeo/licenses"
mkdir -p -- "$destination" "$licenses"
cd -- "$source_dir"
go mod download
go mod verify
go build -mod=readonly -trimpath -buildvcs=false -ldflags='-s -w' -o "$destination/unframeo" ./cmd/unframeo
install -m 0644 LICENSE "$licenses/unframeo-LICENSE"
install -m 0644 "$root/upstream/LICENSE.go" "$licenses/go-LICENSE"
go list -deps -f '{{if .Module}}{{if not .Module.Main}}{{.Module.Path}} {{.Module.Dir}}{{end}}{{end}}' ./cmd/unframeo | sort -u | while read -r module directory; do
    [[ -z "$module" ]] && continue
    name=${module//\//_}
    install -m 0644 "$directory/LICENSE" "$licenses/$name-LICENSE"
    if [[ -f "$directory/PATENTS" ]]; then install -m 0644 "$directory/PATENTS" "$licenses/$name-PATENTS"; fi
done
printf 'Built %s from %s\n' "$architecture" "$revision"
