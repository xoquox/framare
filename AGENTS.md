# Entwicklungshinweise

Framare ist ein SailfishOS-Projekt; App-Quellcode ist noch nicht vorhanden.
`upstream/unframeo-revision` enthält den festgelegten Protokoll-Quellstand.

Versioniert werden App-Quellcode und unmittelbar benötigte Projektdateien.
Lokale Entwicklungsumgebungen, SDK-Helfer, Downloads, Logs und Buildartefakte
bleiben unter `.local/` außerhalb der Versionsverwaltung.
Vor Commits den tatsächlichen Inhalt mit `git diff --cached` prüfen.
