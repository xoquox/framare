# Entwicklungsumgebung

## Voraussetzungen

- Sailfish Application SDK mit `sfdk` und passenden Buildtargets.
- Docker als Build Engine mit funktionierendem Benutzerzugriff. Für den
  Emulator wird zusätzlich VirtualBox benötigt.
- Go ab 1.26 für den festgelegten unframeo-Quellstand.
- Git, Bash, curl, Python 3, SSH, rsync, `file` und `sha256sum`.
- ShellCheck für die Prüfung der Shell-Skripte.

C++/Qt, QML, Silica und RPM-Werkzeuge werden im Sailfish-SDK/Sysroot verwendet.
Betriebssystemspezifische Host-Einrichtung ist nicht Bestandteil des Projekts.

## SDK installieren und prüfen

```sh
bash scripts/download-sdk.sh
bash scripts/install-sdk.sh
bash scripts/check-environment.sh
bash scripts/sfdk.sh --version
bash scripts/sfdk.sh tools list
```

Das Downloadskript lädt SDK 3.13.5 von einer festen offiziellen HTTPS-URL und
prüft Dateigröße, veröffentlichte MD5 und die erfasste SHA256. Downloads und
Installer-Logs liegen unter `.local/` und werden nicht versioniert.

Das Installationsskript benötigt einen für diesen Benutzer erreichbaren
Docker-Dienst und installiert das SDK als normaler Benutzer unter
`~/SailfishOS`. Der unbeaufsichtigte Installer akzeptiert die SDK-Lizenzbedingungen.
Ein bereits vorhandenes SDK wird nicht überschrieben. Komponenten können über
das Maintenance Tool verwaltet werden. Die SDK-Version ist nicht die OS-Zielversion.

`scripts/sfdk.sh` startet standardmäßig `~/SailfishOS/bin/sfdk` und gibt alle
Argumente einzeln weiter. `FRAMARE_SDK_ROOT` ändert das SDK-Verzeichnis.
`FRAMARE_SFDK` kann den absoluten Pfad zu einem ausführbaren lokalen Wrapper
angeben, falls der Host besondere Laufzeitvoraussetzungen hat. Dieser Wrapper
ist lokal bereitzustellen. Der Projektstarter selbst konfiguriert den Host nicht.

## SDK-Testbuild

Der Projekttyp in SDK 3.13.5 heißt `qtquick2app`; verfügbar sind die Vorlagen
über `bash scripts/sfdk.sh init --list-types`. Für einen frischen Test:

```sh
project_root=$PWD
mkdir -p .local/sdk-smoke/harbour-framare-sdkcheck
cd .local/sdk-smoke/harbour-framare-sdkcheck
bash "$project_root/scripts/sfdk.sh" init --type qtquick2app
bash "$project_root/scripts/sfdk.sh" config --push target SailfishOS-5.1.0.11-aarch64
bash "$project_root/scripts/sfdk.sh" build
```

Das Target muss vorher in `tools list` stehen. Für armv7hl ein frisches zweites
Verzeichnis mit gleichem Projektnamen verwenden, damit keine Objektdateien der
anderen Architektur wiederverwendet werden. Diese Vorlage ist keine Framare-App.

## unframeo prüfen

Quelle: https://github.com/borje/unframeo.git

Fester Commit in `upstream/unframeo-revision`:
`149f60b5ac7702f2cbb0761185b0d49a7431bf3f`.

```sh
git clone https://github.com/borje/unframeo.git .local/upstream/unframeo
git -C .local/upstream/unframeo checkout --detach 149f60b5ac7702f2cbb0761185b0d49a7431bf3f
(cd .local/upstream/unframeo && GOTOOLCHAIN=local go test ./...)
bash scripts/build-unframeo-check.sh
```

Das Buildskript verlangt einen sauberen Checkout am festgelegten Commit,
verifiziert die Go-Module und baut mit `CGO_ENABLED=0`, `GOTOOLCHAIN=local`,
`-mod=readonly`, `-trimpath` und `-buildvcs=false` für x86_64, aarch64 und
ARMv7. Ergebnisse und Lizenz-/Herkunftshinweise liegen in
`.local/transport-check/`. Der Host-Starttest ruft nur `help` auf und erzeugt
keine Kopplungsidentität. `protoc` wird für unveränderte generierte Bindings
nicht benötigt. Weitere Befunde stehen in [unframeo.md](unframeo.md).

## Tatsächlicher Prüfstand vom 2026-11-22

- SDK 3.13.5 Stable mit Docker 28.5.2 erfolgreich gestartet.
- Installierte Targets: SailfishOS 5.1.0.11 für aarch64, armv7hl und i486.
- C++/Qt-Quick-Testprojekt mit qmake und deutscher Übersetzung erfolgreich
  für aarch64 und armv7hl als RPM gebaut. Paketname und Architektur mit dem
  RPM-Werkzeug der Build Engine geprüft.
- Testartefakte:
  - `.local/sdk-smoke/harbour-framare-sdkcheck/RPMS/harbour-framare-sdkcheck-0-1.aarch64.rpm`
  - `.local/sdk-smoke/armv7hl/harbour-framare-sdkcheck/RPMS/harbour-framare-sdkcheck-0-1.armv7hl.rpm`
- IDE-Versionsabfrage: Qt Creator 4.15.2 auf Qt 5.15.14 mit SailfishOS-Plugin.
  Maintenance-Tool-Versionsabfrage erfolgreich. Keine interaktiven GUI-Tests.
- Go 1.26.1: alle unframeo-Offline-Tests erfolgreich, Module verifiziert,
  statische ELF-Binaries für x86_64, aarch64 und ARMv7 erzeugt. Nur x86_64 mit
  `help` ausgeführt. Keine Tests mit Build-Tag `live` und keine Kopplung gestartet.
- Shell-Syntax und ShellCheck erfolgreich geprüft.

Die RPMs enthalten ausschließlich die SDK-Vorlage. Kein Framare-Release und
kein vollständig fixierter Release-Build. i486 wurde nicht gebaut. Kein
Emulatorstart, keine RPM-Installation auf einem Telefon und kein Test an einem
Rahmen. Cross-Builds belegen keine Zielsystemausführung, DNS- oder
Sailjail-Kompatibilität.

Beide SDK-Builds meldeten einen fehlenden Pfad `$(get_shared_dir)/devices.xml`.
Beim ARMv7-Build erschienen zusätzlich Scratchbox2-Warnungen zu `statx()` auf
Dateideskriptoren. Beide Builds endeten mit Exitcode 0 und erzeugten die
geprüften RPMs. Deployment auf Geräte ist noch nicht getestet.

## Quellen

- [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/)
- [Installation](https://docs.sailfishos.org/Tools/Sailfish_SDK/Installation/)
- [SDK-Buildkommandos](https://docs.sailfishos.org/Tools/Sailfish_SDK/Building_packages/)
- [unframeo Buildhinweise](https://github.com/borje/unframeo#building)
