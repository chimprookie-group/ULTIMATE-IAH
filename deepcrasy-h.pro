QT += core gui qml quick quickcontrols2 dbus network svg webenginequick

CONFIG += c++17 release
TARGET = deepcrasy-h
TEMPLATE = app

HEADERS += \
    src/acpclient.h \
    src/backend.h \
    src/config.h \
    src/gitstatus.h \
    src/harnessserver.h \
    src/iconprovider.h \
    src/portalfolderpicker.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/acpclient.cpp \
    src/backend.cpp \
    src/config.cpp \
    src/gitstatus.cpp \
    src/harnessserver.cpp \
    src/iconprovider.cpp \
    src/portalfolderpicker.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
