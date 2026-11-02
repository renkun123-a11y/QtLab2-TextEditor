QT += core gui widgets

CONFIG += c++17 console
TEMPLATE = app
TARGET = shots

# 截图工具直接复用主工程的源码，保证截出来的是真实程序的界面。
INCLUDEPATH += ../..

SOURCES += \
    shots.cpp \
    ../../mainwindow.cpp \
    ../../codeeditor.cpp \
    ../../finddialog.cpp \
    ../../aboutdialog.cpp \
    ../../fileio.cpp \
    ../../dialoghelpers.cpp

HEADERS += \
    ../../mainwindow.h \
    ../../codeeditor.h \
    ../../finddialog.h \
    ../../aboutdialog.h \
    ../../fileio.h \
    ../../dialoghelpers.h

FORMS += ../../mainwindow.ui

RESOURCES += ../../resources.qrc
