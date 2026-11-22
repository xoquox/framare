# Geprüfte Protokollgrundlage

Quelle: https://github.com/borje/unframeo

Geprüfter Commit: `149f60b5ac7702f2cbb0761185b0d49a7431bf3f`
(`nix/unframeo-revision`). Keine Upstream-Dateien verändert. Diese Notizen
beschreiben die Grundlage für eine spätere Integration; Framare besitzt noch
keinen Transportadapter.

## Build und Tests

Go-Modul mit Mindestversion 1.26.0; die vorbereitete Umgebung verwendet Go
1.26.1. Generierte Protobuf-Dateien sind eingecheckt. `protoc` und
`protoc-gen-go` werden für einen unveränderten Build nicht benötigt.
`go test ./...` lief mit den lokalen Fake-Rahmen/Fake-Grid-Tests erfolgreich.
Keine Tests mit dem Build-Tag `live` gestartet, kein Rahmen gekoppelt.

Die Buildprüfung erzeugt mit `CGO_ENABLED=0` statische Linux-Binaries für
x86_64, aarch64 und ARMv7. Nur das x86_64-Binary wurde hier mit `help`
ausgeführt. Sailfish-Kernelkompatibilität, DNS und Sailjail sind noch offen.

## CLI-Vertrag für den späteren Adapter

- Optionen stehen vor dem Unterbefehl: `-config`, `-frame`, `-caption`, `-fit`,
  `-timeout`, `-net` und `-discover` sind im Quellcode vorhanden.
- Unterbefehle für den vorgesehenen Umfang: `pair <code>`, `frames`,
  `forget <name>`, `name [<name>]`, `send <file|url>...`.
- `-net` unterstützt `auto`, `local`, `relay`; Standard ist `auto`.
  Lokale Suche standardmäßig 2 Sekunden, Gesamttimeout standardmäßig 15 Minuten.
- Rückgabecode 0 bei Erfolg; `main` beendet Fehlerfälle mit 1. Kein separater
  Exitcode für Netzfehler, ungültigen Code oder Timeout. Menschenlesbare
  Fehlermeldungen dürfen daher nicht als stabile Fehlerklassen gelten.
- `SendPhoto` wartet auf die zum letzten Segment gehörige Rahmenbestätigung
  und prüft deren Fehlerfeld. Das ist eine Annahmebestätigung durch den Rahmen,
  kein Beleg dafür, dass das Bild gerade auf dem Display sichtbar ist.
  Die Bestätigung hat zusätzlich ein Limit von 120 Sekunden.
- Keine strukturierte Byte-Fortschrittsschnittstelle in der CLI festgestellt.
  Die spätere UI benötigt zunächst unbestimmten Fortschritt.
- Mehrere Dateiargumente werden vorab geprüft und nacheinander gesendet; beim
  ersten Sendefehler endet der Prozess. Eine App-Warteschlange pro Bild bleibt
  nötig, damit bestätigte Bilder gezielt von Wiederholungen ausgeschlossen sind.
- Direkte lokale Bilder müssen als Pfad übergeben werden. `file://` wird nicht
  als besondere lokale URL normalisiert. Share-Ressourcen müssen im Backend
  normalisiert und anhand des Inhalts validiert werden. Die CLI ersetzt keine
  vollständige Bilddekodierung oder EXIF-Behandlung durch die App.

## Identität und Lizenz

`-config` überschreibt den Konfigurationspfad explizit; alternativ existiert
`UNFRAMEO_CONFIG`. Die Datei enthält den privaten Identitätsschlüssel,
Sendernamen, Rahmen und Standardrahmen. Nur `pair` erzeugt eine fehlende
Identität; beschädigte Konfigurationen werden nicht stillschweigend ersetzt.
Speichern verwendet ein temporäres Geschwisterfile mit Zielmodus 0600 und
Rename. Gleichzeitige Prozesse müssen vom App-Backend serialisiert werden.
`forget` entfernt ausschließlich den lokalen Eintrag; es löscht keine Fotos.

`LICENSE` enthält GPLv3; `NOTICE` und SPDX-Header nennen GPL-3.0-or-later,
die opensdg-Portierung und BSD-Abhängigkeiten. Die lokalen Buildartefakte
enthalten Kopien von LICENSE/NOTICE und einen Quellenverweis. Für ein späteres
RPM müssen die vollständigen Lizenzpflichten aller mitgelieferten Bestandteile
berücksichtigt werden.

Nachprüfbare Quellstellen im festgelegten Commit:

- [CLI und Exitcodes](https://github.com/borje/unframeo/blob/149f60b5ac7702f2cbb0761185b0d49a7431bf3f/cmd/unframeo/main.go)
- [Übertragung und Bestätigung](https://github.com/borje/unframeo/blob/149f60b5ac7702f2cbb0761185b0d49a7431bf3f/internal/frameo/media.go)
- [Konfiguration](https://github.com/borje/unframeo/blob/149f60b5ac7702f2cbb0761185b0d49a7431bf3f/internal/config/config.go)
- [Herkunftshinweise](https://github.com/borje/unframeo/blob/149f60b5ac7702f2cbb0761185b0d49a7431bf3f/NOTICE)
