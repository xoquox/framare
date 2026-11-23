# Entwicklungshinweise

Framare ist eine native SailfishOS-App mit QML/Silica und C++/Qt-Backend.
`src/` enthält Prozessverwaltung, private Einstellungen und Bildwarteschlange;
`qml/` die Oberfläche und den ShareProvider. Technische IDs stehen in `app.pri`.
`rpm/` enthält das Packaging, `tests/` deterministische Backend-Tests.
`upstream/unframeo-revision` pinnt den unveränderten Go-Transport.

Build und Tests sind in README.md beschrieben. Qt-Backend-Tests laufen mit
Fake-Transport ohne Netzwerk oder echte Kopplungsdaten. Prozessargumente
einzeln übergeben; CLI-Ausgaben enthalten private Daten und werden verworfen.
Identität und Bestätigungsstatus bei Änderungen erhalten. SDK-Cross-Builds,
QML-Ladetests und echte Gerätetests getrennt ausweisen.

Versioniert werden App-Quellcode und unmittelbar benötigte Projektdateien.
Lokale Entwicklungsumgebungen, SDK-Helfer, Downloads, Logs und Buildartefakte
bleiben unter `.local/` außerhalb der Versionsverwaltung.
Vor Commits den tatsächlichen Inhalt mit `git diff --cached` prüfen.
