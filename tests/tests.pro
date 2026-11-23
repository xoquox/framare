TEMPLATE = app
TARGET = tst_framare
QT += core gui concurrent testlib
CONFIG += console testcase c++11
CONFIG -= app_bundle
SOURCES += tst_framare.cpp ../src/framare.cpp
HEADERS += ../src/framare.h
INCLUDEPATH += ../src
DEFINES += FIXTURE_DIR=\\\"$$PWD\\\"
