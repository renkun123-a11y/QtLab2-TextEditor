QT += core gui widgets

CONFIG += c++17
TEMPLATE = app
TARGET = TextEditor

# 源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    codeeditor.cpp \
    finddialog.cpp \
    aboutdialog.cpp \
    fileio.cpp \
    dialoghelpers.cpp

HEADERS += \
    mainwindow.h \
    codeeditor.h \
    finddialog.h \
    aboutdialog.h \
    fileio.h \
    dialoghelpers.h \
    appinfo.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
