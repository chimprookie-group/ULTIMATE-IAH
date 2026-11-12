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
    src/connectiondoctor.h \
    src/installationdoctor.h \
    src/agentconversationstore.h \
    src/githubservice.h \
    src/webservice.h \
    src/pluginmanager.h \
    src/notestore.h \
    src/ultimateagent.h \
    src/hoverservice.h \
    src/botmanager.h \
    src/botserver.h \
    src/botrunner.h \
    src/botstore.h \
    src/mcpmanager.h \
    src/skillmanager.h \
    src/agentmanager.h \
    src/autopilot.h \
    src/iconprovider.h \
    src/portalfolderpicker.h \
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
    src/connectiondoctor.cpp \
    src/installationdoctor.cpp \
    src/agentconversationstore.cpp \
    src/githubservice.cpp \
    src/webservice.cpp \
    src/pluginmanager.cpp \
    src/notestore.cpp \
    src/ultimateagent.cpp \
    src/hoverservice.cpp \
    src/botmanager.cpp \
    src/botserver.cpp \
    src/botrunner.cpp \
    src/botstore.cpp \
    src/mcpmanager.cpp \
    src/skillmanager.cpp \
    src/agentmanager.cpp \
    src/autopilot.cpp \
    src/iconprovider.cpp \
    src/portalfolderpicker.cpp \
    src/terminalsession.cpp \
    src/theme.cpp \
    src/workspacemodel.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
LIBS += -lutil
