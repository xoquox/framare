# Entwicklungshinweise

## Stand und Struktur

Das Repository enthält SDK-Hilfsskripte und Buildprüfungen für Framare.
App-Quellcode und Release-Packaging sind noch nicht vorhanden. Die SDK-Testprojekte
unter `.local/` sind keine Framare-Implementierung.

- `upstream/unframeo-revision`: festgelegter unframeo-Commit.
- `scripts/`: SDK-Download, Installation, Starter und Buildprüfungen.
- `docs/environment.md`: Voraussetzungen, Buildkommandos und Prüfergebnisse.
- `docs/unframeo.md`: geprüfter Upstream-Stand und technische Befunde.
- `.local/`: ignorierte Downloads, Checkouts, Logs und Testartefakte.

## Entwicklung und Prüfung

Geprüft wurden SDK 3.13.5, SailfishOS-5.1.0.11-Targets und Go 1.26.1.
SDK-Test-RPMs wurden für aarch64 und armv7hl gebaut.

```sh
shellcheck scripts/*.sh
bash scripts/check-environment.sh
bash scripts/sfdk.sh tools list
(cd .local/upstream/unframeo && GOTOOLCHAIN=local go test ./...)
bash scripts/build-unframeo-check.sh
```

`scripts/sfdk.sh` verwendet standardmäßig `~/SailfishOS/bin/sfdk`.
`FRAMARE_SDK_ROOT` wählt ein anderes SDK-Verzeichnis; `FRAMARE_SFDK` kann ein
lokales ausführbares Wrapper-Skript angeben. Host-spezifische Einrichtungen
gehören außerhalb der versionierten Projektdateien.

## Änderungen

Bei Änderungen am Upstream-Pin Herkunft, Lizenzprüfung und Buildtests abgleichen.
Build-Erfolge, Geräteausführung und echte Rahmentests getrennt dokumentieren.
Keine lokalen SDKs, RPMs, Logs, privaten Konfigurationen oder Schlüssel committen.
Vor Commits den Staging-Bereich mit `git diff --cached` prüfen.
