QT += core gui qml quick quick-private quickcontrols2 dbus network sql svg webchannel webenginequick

CONFIG += c++17 release
TARGET = deepcrasy-h
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/config.h \
    src/ecosystem.h \
    src/gitstatus.h \
    src/harnessserver.h \
    src/harnessconnections.h \
    src/memorystore.h \
    src/autopilot.h \
    src/iconprovider.h \
    src/portalfolderpicker.h \
    src/nineroutermanager.h \
    src/mausbotmanager.h \
    src/terminalsession.h \
    src/theme.h \
    src/workspacemodel.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/config.cpp \
    src/ecosystem.cpp \
    src/gitstatus.cpp \
    src/harnessserver.cpp \
    src/harnessconnections.cpp \
    src/memorystore.cpp \
    src/autopilot.cpp \
    src/iconprovider.cpp \
    src/portalfolderpicker.cpp \
    src/nineroutermanager.cpp \
    src/mausbotmanager.cpp \
    src/terminalsession.cpp \
    src/theme.cpp \
    src/workspacemodel.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
LIBS += -lutil
