QT += core gui qml quick quick-private quickcontrols2 dbus network svg webchannel webenginequick

CONFIG += c++17 release
TARGET = deepcrasy-h
TEMPLATE = app

HEADERS += \
    src/acpclient.h \
    src/backend.h \
    src/config.h \
    src/ecosystem.h \
    src/gitstatus.h \
    src/harnessserver.h \
    src/mcpmanager.h \
    src/messagemodel.h \
    src/iconprovider.h \
    src/portalfolderpicker.h \
    src/terminalsession.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/acpclient.cpp \
    src/backend.cpp \
    src/config.cpp \
    src/ecosystem.cpp \
    src/gitstatus.cpp \
    src/harnessserver.cpp \
    src/mcpmanager.cpp \
    src/messagemodel.cpp \
    src/iconprovider.cpp \
    src/portalfolderpicker.cpp \
    src/terminalsession.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
LIBS += -lutil
