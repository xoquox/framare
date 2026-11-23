# Framare

SailfishOS-Projekt zum Senden von Bildern an Frameo-kompatible Bilderrahmen.
Paketname: `harbour-framare`.

Erste native Testversion mit Rahmenverwaltung, Sendernamen, Galerie-Share,
Bildvorschau, Beschriftung und Einpassen/Zuschneiden. Mehrere Bilder werden
nacheinander gesendet; fehlgeschlagene Bilder können einzeln erneut gesendet
werden. Originale bleiben unverändert. Oberfläche auf Deutsch und Englisch.

Im Menü „Rahmen und Absender“ einen Rahmen mit seinem aktuellen
Freundschaftscode koppeln. Danach Bilder aus der Galerie mit Framare teilen
oder über „Bild hinzufügen“ auswählen. Ziel und Darstellung wählen und senden.
„Lokal vergessen“ entfernt die lokale Kopplung und löscht keine Rahmenfotos.

## Build

Benötigt werden Go **1.26.1**, Git und das Sailfish SDK. Geprüfte SDK-Version:
3.13.5; Geräte-Buildbasis: **SailfishOS 5.0.0.62**. Native Teile werden im
Sailfish-Sysroot gebaut, das mitgelieferte Go-Programm vorher auf dem Host.

```sh
bash upstream/build.sh aarch64
sfdk config --push target SailfishOS-5.0.0.62-aarch64
sfdk build
```

Für armv7hl das Transport-Binary mit `bash upstream/build.sh armv7hl` bauen
und das entsprechende SDK-Target verwenden. Beim Architekturwechsel zuerst
mit `sfdk build-shell make clean` die Objektdateien entfernen. Die RPMs liegen
in `RPMS/`; zum manuellen Test auf das Telefon kopieren und dort installieren.
Lokale SDK-Einrichtung, Downloads und Artefakte bleiben außerhalb von Git.

Die Launcher-PNGs unter `icons/<Größe>/` werden aus `icons/harbour-framare.svg`
exportiert. Bei Änderungen am SVG alle vier Größen aktualisieren, beispielsweise
mit `inkscape icons/harbour-framare.svg --export-width=86 --export-height=86 --export-filename=icons/86x86/harbour-framare.png`
(entsprechend auch für 108, 128 und 172 Pixel).

## Tests und Grenzen

Backend-Tests benötigen Qt 5 Core/Gui/Concurrent/Test und Python 3:

```sh
mkdir -p .local/tests
cd .local/tests
qmake ../../tests/tests.pro
make
./tst_framare
```

12 Qt-Testfälle bestehen mit Fake-Transport, darunter Argumentübergabe mit
Sonderzeichen, Rahmenauswahl, Warteschlange, Wiederholung, Prozessabbruch,
Timeout, Identitätsschutz und EXIF-Ausrichtung. Die QML-Seiten einschließlich
ShareProvider-Registrierung wurden in einer isolierten SDK-D-Bus-Sitzung geladen.
Dies ersetzt keinen Telefon- oder echten Rahmentest. Galerie-Startverhalten,
Sailjail einschließlich Speicherkarte, echtes Koppeln und lokaler/entfernter
Versand sind am Gerät noch zu prüfen. Keine Harbour-Zulassung vorausgesetzt.

JPEG/PNG, maximal 32 MB und 40 Megapixel pro Eingabebild, 16 Bilder pro
Warteschlange und 128 MB privater Bildcache. Vorschau und Versand verwenden
ausgerichtete private JPEG-Kopien. Die Warteschlange gilt für die laufende App;
kein Hintergrunddienst und keine zugesicherte Weiterübertragung bei geschlossener
oder angehaltener App. Bei Abbruch oder fehlender Bestätigung kann der Empfang
unklar sein. Bestätigte Bilder werden nicht automatisch erneut gesendet.

Die private Kopplungsidentität bleibt im App-Datenverzeichnis
`~/.local/share/harbour/framare/identity.json` erhalten. Eine fehlende frühere
oder beschädigte Identität wird nicht automatisch ersetzt.

## Herkunft und Lizenz

GPL-3.0-or-later. Der Transport stammt unverändert aus
[unframeo](https://github.com/borje/unframeo), festgelegt in
[upstream/unframeo-revision](upstream/unframeo-revision). Attributionen stehen
in [upstream/NOTICE.unframeo](upstream/NOTICE.unframeo); das Buildskript sammelt
auch Go- und Abhängigkeitslizenzen für das RPM. Framare ist ein unabhängiges
Projekt ohne offizielle Verbindung zu Frameo, Jolla oder Trifork.
