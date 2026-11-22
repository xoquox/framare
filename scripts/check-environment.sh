#!/usr/bin/env bash
set -uo pipefail
missing=0
report_missing() { printf 'FEHLT: %s\n' "$*"; missing=$((missing + 1)); }
for tool in git nix nix-build curl python3 ssh rsync; do
  if command -v "$tool" >/dev/null; then
    printf 'OK: %s\n' "$tool"
  else
    report_missing "$tool"
  fi
done
if command -v go >/dev/null; then
  GOTOOLCHAIN=local go version
  go_version=$(GOTOOLCHAIN=local go env GOVERSION) || go_version=unknown
  if [[ ! $go_version =~ ^go([0-9]+)\.([0-9]+) ]] ||
     (( BASH_REMATCH[1] < 1 || (BASH_REMATCH[1] == 1 && BASH_REMATCH[2] < 26) )); then
    report_missing 'Go >= 1.26 für den derzeit geprüften unframeo-Stand'
  fi
else
  report_missing 'Go >= 1.26'
fi
if command -v docker >/dev/null; then
  if docker info >/dev/null 2>&1; then
    printf 'OK: Docker engine erreichbar\n'
  else
    report_missing 'Docker engine nicht erreichbar (Dienst / Gruppenmitgliedschaft prüfen)'
  fi
else
  report_missing 'Docker'
fi
sdk_root=${FRAMARE_SDK_ROOT:-"$HOME/SailfishOS"}
if [[ -x "$sdk_root/bin/sfdk" ]]; then
  printf 'GEFUNDEN: sfdk unter %s; Start und Targets separat in der FHS-Umgebung prüfen\n' "$sdk_root"
else
  report_missing "Sailfish SDK unter $sdk_root"
fi
printf 'Offene Voraussetzungen: %s\n' "$missing"
(( missing == 0 ))
