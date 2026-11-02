QT += core gui widgets testlib

CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_editor

INCLUDEPATH += ..

SOURCES += \
    tst_editor.cpp \
    ../mainwindow.cpp \
    ../codeeditor.cpp \
    ../finddialog.cpp \
    ../aboutdialog.cpp \
    ../fileio.cpp \
    ../dialoghelpers.cpp

HEADERS += \
    ../mainwindow.h \
    ../codeeditor.h \
    ../finddialog.h \
    ../aboutdialog.h \
    ../fileio.h \
    ../dialoghelpers.h \
    ../appinfo.h

FORMS += ../mainwindow.ui

RESOURCES += ../resources.qrc
