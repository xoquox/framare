# Entwicklungshinweise

## Stand und Struktur

Das Repository enthält die Entwicklungsumgebung für Framare. App-Quellcode und
Release-Packaging sind noch nicht vorhanden. Die SDK-Testprojekte unter
`.local/` sind keine Framare-Implementierung.

- `nix/`: NixOS-Modul, SDK-FHS-Umgebung und unframeo-Commit-Pin.
- `shell.nix`: projektbezogene Go- und Diagnosewerkzeuge.
- `scripts/`: Einrichtung, SDK-Starter und Buildprüfungen.
- `docs/environment.md`: Einrichtung, Buildkommandos und tatsächliche Prüfergebnisse.
- `docs/unframeo.md`: geprüfter Upstream-Stand und technische Befunde.
- `.local/`: ignorierte Downloads, Checkouts, Logs und Testartefakte.

## Entwicklung und Prüfung

Die geprüfte Umgebung verwendet SDK 3.13.5, SailfishOS-5.1.0.11-Targets und
Go 1.26.1. SDK-Test-RPMs wurden für aarch64 und armv7hl gebaut.

```sh
nix-shell --run 'shellcheck scripts/*.sh'
bash scripts/check-environment.sh
bash scripts/sfdk.sh tools list
nix-shell --run 'cd .local/upstream/unframeo && go test ./...'
nix-shell --run 'bash scripts/build-unframeo-check.sh'
```

SDK-Binaries unter NixOS über `scripts/sfdk.sh` oder die FHS-Umgebung starten.
Docker muss laufen und die Gruppenmitgliedschaft wirksam sein. In bestehenden
Sitzungen kann `sg docker -c '…'` verwendet werden.

## Änderungen

Host-Konfigurationsänderungen zuerst mit `python3 scripts/configure-host.py`
anzeigen. Gewöhnliche Quellcodeprüfungen benötigen keine Systemänderungen.
Bei Änderungen am Upstream-Pin Herkunft, Lizenzprüfung und Buildtests abgleichen.
Build-Erfolge, Geräteausführung und echte Rahmentests getrennt dokumentieren.
Keine lokalen SDKs, RPMs, Logs, privaten Konfigurationen oder Schlüssel committen.
Vor Commits den Staging-Bereich mit `git diff --cached` prüfen.
