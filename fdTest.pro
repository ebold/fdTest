#-------------------------------------------------
#
# Project created by QtCreator 2015-07-09T15:50:38
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fdTest
TEMPLATE = app

INCLUDEPATH += $$PWD/gui
INCLUDEPATH += $$PWD/protocols
INCLUDEPATH += $$PWD/utils

include(gui/gui.pri)
include(protocols/protocol.pri)
include(utils/utils.pri)

SOURCES += main.cpp

HEADERS  += fd.h

RESOURCES += resources.qrc

TRANSLATIONS = fdTest_de.ts

