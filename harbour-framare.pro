include(app.pri)
TARGET = $$APP_ID
CONFIG += sailfishapp sailfishapp_i18n c++11
SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172
QT += quick dbus concurrent
SOURCES += src/main.cpp src/framare.cpp
HEADERS += src/framare.h
TRANSLATIONS += translations/harbour-framare-de.ts
DISTFILES += qml/harbour-framare.qml qml/pages/*.qml icons/harbour-framare.svg

isEmpty(UNFRAMEO_BINARY): error("Build the pinned transport and pass UNFRAMEO_BINARY to qmake")
!exists($$UNFRAMEO_BINARY): error("The unframeo binary is missing; run upstream/build.sh for the target architecture")
transport.files = $$UNFRAMEO_BINARY
transport.path = /usr/libexec/$$APP_ID
INSTALLS += transport
