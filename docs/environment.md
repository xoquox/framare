# Entwicklungsumgebung

## Entscheidung

- Dauerhaft über NixOS: Docker Build Engine und Go 1.26. Das vorhandene Nixpkgs
  bietet `go_1_26` 1.26.1; `pkgs.go` ist dagegen 1.25.7 und reicht für das am
  2026-11-22 geprüfte unframeo-`go.mod` nicht aus.
- Projektbezogen: Diagnosewerkzeuge über `nix-shell`; die Laufzeitbibliotheken
  für den vorgebauten SDK-Installer und `sfdk` über `buildFHSEnv`.
- Sailfish SDK 3.13.5 als normale Benutzerinstallation, keine Root-Installation.
  Zunächst Docker; VirtualBox erst bei Bedarf für den Emulator.
- C++/Qt, QML, Silica, ShareProvider und RPM-Packaging im Sailfish-Sysroot.
  Kein zusätzliches allgemeines Host-Qt als Ersatz für das SDK.

NixOS ist kein ausdrücklich verifizierter SDK-Host. Der SDK-Installer wurde
in der FHS-Umgebung erfolgreich gestartet. Rootless Docker ist laut
SDK-Dokumentation nicht unterstützt.

## Dauerhafte Host-Konfiguration

`nix/nixos-development.nix` ergänzt die bestehende Konfiguration; vorhandene
Pakete, Gruppen und die VMware-Konfiguration werden nicht ersetzt.

Vom Projektverzeichnis aus zunächst die konkrete Änderung anzeigen:

```sh
python3 scripts/configure-host.py
```

Anschließend im lokalen Terminal mit sudo anwenden:

```sh
sudo "$(command -v python3)" scripts/configure-host.py --apply
sudo nixos-rebuild build && sudo nixos-rebuild switch
```

Das Skript sichert die bestehende Konfiguration, installiert das Modul unter
`/etc/nixos/framare-development.nix`, ergänzt den vorhandenen `imports`-Block
und trägt auf oberster Ebene ein:

```nix
framareDevelopment = {
  enable = true;
  user = "<lokaler-benutzer>";
};
```

Das Skript ermittelt den aufrufenden Benutzer, auch unter sudo. Ein anderer
Entwicklungsbenutzer kann mit `--user NAME` gewählt werden.
Danach neu anmelden, damit die Docker-Gruppe wirksam wird. Die Gruppe erlaubt
weitreichende Host-Verwaltung; sie wird benötigt, damit das SDK die reguläre
Docker Engine ohne Root-Start des SDK bedienen kann. Es wird kein Docker-
TCP-Port geöffnet. `docker info` muss als normaler Benutzer funktionieren.
In einer bestehenden Sitzung kann `sg docker -c 'docker info'` die bereits
eingetragene Gruppenmitgliedschaft nutzen, ohne Root-Start des SDK.

## Repository initialisieren

```sh
bash scripts/init-repository.sh
```

Das Skript erstellt nur ein lokales Repository mit Branch `main`, keinen
Commit und kein Remote. Vor Commits gezielt Dateien stagen und
`git diff --cached` auf Inhalt und vertrauliche Daten prüfen.

## SDK laden und starten

```sh
nix-shell
bash scripts/download-sdk.sh
bash scripts/sdk-env.sh
```

Der Download verwendet eine feste offizielle URL, prüft die veröffentlichte
Dateigröße und MD5 sowie die beim ersten Download erfasste SHA256. MD5 dient ausschließlich
dem Abgleich mit der veröffentlichten Datei; die Quelle wird per HTTPS
abgerufen. Installer liegen in `.local/downloads/` und werden nicht versioniert.

In der FHS-Shell vom Projektverzeichnis aus den grafischen Installer starten:

```sh
./.local/downloads/SailfishSDK-3.13.5-linux64-online.run
```

Docker als Build Engine wählen, SDK standardmäßig unter `~/SailfishOS`
installieren. Die vom Installer angebotenen stabilen Targets für `aarch64`
und `armv7hl` auswählen, soweit verfügbar; ihre konkreten Versionsnamen nach
Installation protokollieren. Die SDK-Version ist nicht die SailfishOS-Zielversion.

Alternativ dokumentiert Sailfish diese unbeaufsichtigte Installation, welche
auch die Lizenzbedingungen akzeptiert:

```sh
QT_QPA_PLATFORM=minimal ./.local/downloads/SailfishSDK-3.13.5-linux64-online.run non-interactive=1 accept-licenses=1 build-engine-type=docker
```

Das projektspezifische Skript prüft vorher Docker, schreibt das Installer-Log
nach `.local/logs/sdk-install.log` und erkennt auch einen fehlgeschlagenen
Installationsversuch mit Exitcode 0:

```sh
bash scripts/install-sdk.sh
# Bei noch nicht erneuerter Login-Sitzung stattdessen:
sg docker -c 'bash scripts/install-sdk.sh'
```

Nicht parallel mehrfach installieren. Bei unvollständiger Installation zuerst
Installer-Ausgabe und Maintenance Tool prüfen. SDK-Binaries werden weiter in
der FHS-Shell ausgeführt, ihr `bin`-Verzeichnis nicht pauschal dem PATH beigefügt.

Nach erfolgreicher Installation die Menüeinträge für NixOS einrichten:

```sh
bash scripts/configure-sdk-launchers.sh
```

Das Skript sichert die ursprünglichen Desktop-Dateien im SDK-Verzeichnis,
leitet IDE und Maintenance Tool durch die FHS-Umgebung und hält diese über
`~/SailfishOS/framare-nix-env` als Nix-GC-Root verfügbar. Die Starter funktionieren
damit auch unabhängig vom Projektcheckout. Nach einem SDK-Update Menüeinträge
kontrollieren und das Skript bei Bedarf erneut ausführen.

## Umgebung testen

Außerhalb der FHS-Shell:

```sh
bash scripts/check-environment.sh
```

Für einen abweichenden SDK-Pfad `FRAMARE_SDK_ROOT` setzen. Der Check prüft nur
Voraussetzungen und Docker-Zugriff; ein gefundenes SDK ist noch kein Starttest.

Für `sfdk` gibt es einen Wrapper, der die FHS-Umgebung verwendet und alle
Argumente einzeln weitergibt:

```sh
bash scripts/sfdk.sh --version
bash scripts/sfdk.sh tools list
bash scripts/sfdk.sh init --list-types
```

Der tatsächliche Projekttyp im SDK 3.13.5 heißt `qtquick2app`. Die offizielle
Build-Anleitung nennt noch `qtquick2-app`; das wurde vom installierten SDK
abgelehnt. Für einen frischen, isolierten Test vom Projektverzeichnis aus:

```sh
project_root=$PWD
mkdir -p .local/sdk-smoke/harbour-framare-sdkcheck
cd .local/sdk-smoke/harbour-framare-sdkcheck
bash "$project_root/scripts/sfdk.sh" init --type qtquick2app
bash "$project_root/scripts/sfdk.sh" config --push target SailfishOS-5.1.0.11-aarch64
bash "$project_root/scripts/sfdk.sh" build
```

Für `SailfishOS-5.1.0.11-armv7hl` ein zweites, frisches Verzeichnis mit gleichem
Projektnamen verwenden, damit keine Objektdateien der anderen Architektur
wiederverwendet werden. Beide Targets müssen vorher in `tools list` stehen.
In einer älteren Login-Sitzung kann der ganze Aufruf mit `sg docker -c '…'`
erfolgen. Das ist ein SDK-Testprojekt, keine Framare-Implementierung.

## unframeo-Buildprüfung

Quelle: `https://github.com/borje/unframeo.git`, fester Commit in
`nix/unframeo-revision`: `149f60b5ac7702f2cbb0761185b0d49a7431bf3f`.
Der Checkout liegt unversioniert unter `.local/upstream/unframeo`.
Bei einer neuen Umgebung:

```sh
git clone https://github.com/borje/unframeo.git .local/upstream/unframeo
git -C .local/upstream/unframeo checkout --detach 149f60b5ac7702f2cbb0761185b0d49a7431bf3f
nix-shell --run 'cd .local/upstream/unframeo && go test ./...'
nix-shell --run 'bash scripts/build-unframeo-check.sh'
```

Das Buildskript verlangt den exakten Commit und einen sauberen Checkout,
prüft die Modulprüfsummen und baut mit `CGO_ENABLED=0`, `GOTOOLCHAIN=local`,
`-mod=readonly`, `-trimpath` und `-buildvcs=false` für x86_64, aarch64 und
ARMv7. Ergebnisse einschließlich Lizenz-/Herkunftshinweisen liegen unter
`.local/transport-check/`. Der Host-Starttest ruft nur `help` auf und erzeugt
keine Kopplungsidentität. Pure-Go-Cross-Builds
belegen keine Sailfish-Lauffähigkeit: DNS, Prozessstart unter Sailjail und
echter Versand benötigen anschließend Gerät/Emulator beziehungsweise Rahmen.
Kein `@latest` im späteren Release-Build. `protoc` wird für unveränderte,
bereits generierte Protobuf-Bindings nicht benötigt.

## Tatsächlich geprüft am 2026-11-22

- Host: NixOS 25.11, x86_64; anfangs etwa 49 GB frei, 62 GiB sichtbarer RAM.
- Git 2.47.2, Make 4.2.1, CMake 3.30.5, Ninja 1.12.1, Python 3.12.8,
  Nix 2.31.2 sowie curl, SSH, rsync und pkg-config vorhanden.
- Lokales Nixpkgs: `fea3b367d61c1a6592bc47c72f40a9f3e6a53e96`.
  Go 1.26.1 und Docker 28.5.2 wurden über das NixOS-Modul dauerhaft eingerichtet.
- Docker-Dienst aktiv, Zugriff als Benutzer mit `sg docker` erfolgreich.
- Lokales Git-Repository auf Branch `main` angelegt.
- SDK-Installer heruntergeladen, Größe und veröffentlichte MD5 geprüft.
  SHA256: `1b5703da664a05743ffa93e68cff874f17f21076358adeb85145bd62bf0ddb86`.
- FHS-Umgebung gebaut, fehlende Bibliotheken anhand von `ldd` ergänzt,
  Installer mit `QT_QPA_PLATFORM=minimal` und `--help` erfolgreich gestartet.
- Erster Installationsversuch ohne Docker-Dienst endete mit einer
  Docker-Voraussetzungsfehlermeldung bei Exitcode 0. Die SDK-Installation wurde
  nach Aktivierung der Docker Engine erfolgreich abgeschlossen.
- SDK 3.13.5 Stable unter `~/SailfishOS` installiert. `sfdk --version` und
  `sfdk tools list` erfolgreich. Build Engine startet als Docker-Container
  `sailfish-sdk-build-engine_<benutzer>`.
- Installierte Targets: `SailfishOS-5.1.0.11-aarch64`,
  `SailfishOS-5.1.0.11-armv7hl`, `SailfishOS-5.1.0.11-i486`.
- C++/Qt-Quick-Testprojekt mit qmake einschließlich deutscher Übersetzung
  erfolgreich für aarch64 und armv7hl gebaut. Beide RPMs mit dem RPM-Werkzeug
  der Build Engine auf Namen und Architektur geprüft:
  - `.local/sdk-smoke/harbour-framare-sdkcheck/RPMS/harbour-framare-sdkcheck-0-1.aarch64.rpm`
  - `.local/sdk-smoke/armv7hl/harbour-framare-sdkcheck/RPMS/harbour-framare-sdkcheck-0-1.armv7hl.rpm`
- Menü-Starter eingerichtet. IDE-Versionsabfrage erfolgreich: Qt Creator
  4.15.2 auf Qt 5.15.14 mit SailfishOS-Plugin. Maintenance-Tool-Versionsabfrage
  ebenfalls erfolgreich. Dies waren Starts mit `QT_QPA_PLATFORM=minimal`, keine
  interaktiven GUI-Tests.
- Abschließender Umgebungscheck mit Docker-Gruppe: keine offenen Voraussetzungen.
- unframeo-Offline-Tests (`go test ./...`) erfolgreich, alle Module verifiziert.
  Builds für x86_64, aarch64 und ARMv7 erfolgreich. `file` erkennt alle drei
  als statisch gelinkte ELF-Dateien; `unframeo-amd64 help` erfolgreich.
- `LICENSE` und `NOTICE` am festgelegten unframeo-Commit gelesen: GPL-3.0-or-later,
  Attribution der opensdg-Portierung und Hinweise auf BSD-Abhängigkeiten.
- Alle Shell-Skripte mit `bash -n` und alle Nix-Dateien mit
  `nix-instantiate --parse` geprüft. ShellCheck erfolgreich. Das Konfigurations-
  Hilfsskript wurde auf Idempotenz und Ablehnung unbekannter Importstrukturen
  geprüft; bestehende VMware-Einstellungen bleiben erhalten.
- Modul mit der vorhandenen `/etc/nixos/configuration.nix` ausgewertet:
  Docker aktiviert, Go 1.26.1 ausgewählt, bestehende Benutzergruppen erhalten
  und um `docker` ergänzt, VMware weiterhin aktiviert. Die Konfiguration wurde
  anschließend im lokalen Terminal angewendet; Docker und Go sind aktiv.

Die Nix-Ausdrücke verwenden zunächst das vorhandene `<nixpkgs>`. Der obige
Revisionseintrag dokumentiert den geprüften Paketstand; es gibt noch keinen
vollständig fixierten Release-Build. Die erzeugten RPMs enthalten ausschließlich
die SDK-Vorlage, keinen Framare-Sender. i486 ist installiert, aber nicht gebaut.
Kein Emulatorstart, keine RPM-Installation auf einem Telefon und kein Test an
einem echten Rahmen wurden durchgeführt. Die ARM-Go-Binaries wurden auf diesem
Host nur gebaut, nicht ausgeführt. VirtualBox wurde nicht installiert.

Während beider erfolgreichen SDK-Builds meldete `mb2` einen fehlenden Pfad
`$(get_shared_dir)/devices.xml`. Beim ARMv7-Build kamen Scratchbox2-Warnungen
zu `statx()` auf Dateideskriptoren hinzu. Beide Builds endeten mit Exitcode 0
und erzeugten die geprüften RPMs. Geräte-Konfiguration und Deployment sind
noch nicht getestet; die SDK-Werkzeuge wurden nicht gepatcht.

## Quellen

- [SDK und Download](https://docs.sailfishos.org/Tools/Sailfish_SDK/)
- [Installation und Host-Voraussetzungen](https://docs.sailfishos.org/Tools/Sailfish_SDK/Installation/)
- [SDK-Testprojekt und Buildkommandos](https://docs.sailfishos.org/Tools/Sailfish_SDK/Building_packages/)
- [unframeo go.mod](https://github.com/borje/unframeo/blob/main/go.mod)
- [unframeo Buildhinweise](https://github.com/borje/unframeo#building)
