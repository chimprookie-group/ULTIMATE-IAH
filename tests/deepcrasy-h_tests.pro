QT += network core gui sql testlib
CONFIG += c++17 testcase
TARGET = ultimate-iah-tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/config.h \
    ../src/ecosystem.h \
    ../src/harnessserver.h \
    ../src/harnessconnections.h \
    ../src/memorystore.h \
    ../src/missionrunner.h \
    ../src/mcpmanager.h \
    ../src/skillmanager.h \
    ../src/agentmanager.h \
    ../src/autopilot.h \
    ../src/botstore.h \
    ../src/agentconversationstore.h \
    ../src/notestore.h \
    ../src/webservice.h \
    ../src/updateservice.h \
    ../src/crasyrouter.h \
    ../src/pluginmanager.h \
    ../src/terminalsession.h \
    ../src/theme.h \
    ../src/workspacemodel.h \
    ../src/translator.h

LIBS += -lutil

SOURCES += \
    deepcrasy-h_tests.cpp \
    ../src/config.cpp \
    ../src/ecosystem.cpp \
    ../src/gitstatus.cpp \
    ../src/terminalsession.cpp \
    ../src/harnessserver.cpp \
    ../src/harnessconnections.cpp \
    ../src/memorystore.cpp \
    ../src/missionrunner.cpp \
    ../src/mcpmanager.cpp \
    ../src/skillmanager.cpp \
    ../src/agentmanager.cpp \
    ../src/autopilot.cpp \
    ../src/botstore.cpp \
    ../src/agentconversationstore.cpp \
    ../src/notestore.cpp \
    ../src/webservice.cpp \
    ../src/updateservice.cpp \
    ../src/crasyrouter.cpp \
    ../src/pluginmanager.cpp \
    ../src/theme.cpp \
    ../src/workspacemodel.cpp \
    ../src/translator.cpp

RESOURCES += ../src/resources.qrc
