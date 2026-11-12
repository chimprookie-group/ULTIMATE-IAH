TARGET = ultimate-hai
QT += core gui qml quick quick-private quickcontrols2 dbus network sql svg webchannel webenginequick

CONFIG += c++17 release
TARGET = ultimate-hai
TEMPLATE = app

HEADERS += \
    src/designtokens.h \
    src/backend.h \
    src/config.h \
    src/ecosystem.h \
    src/gitstatus.h \
    src/harnessserver.h \
    src/harnessconnections.h \
    src/memorystore.h \
    src/missionrunner.h \
    src/mcpmanager.h \
    src/skillmanager.h \
    src/agentmanager.h \
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
    src/designtokens.cpp \
    src/backend.cpp \
    src/config.cpp \
    src/ecosystem.cpp \
    src/gitstatus.cpp \
    src/harnessserver.cpp \
    src/harnessconnections.cpp \
    src/memorystore.cpp \
    src/missionrunner.cpp \
    src/mcpmanager.cpp \
    src/skillmanager.cpp \
    src/agentmanager.cpp \
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
