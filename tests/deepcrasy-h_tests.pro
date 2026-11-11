QT += core gui testlib
CONFIG += c++17 testcase
TARGET = deepcrasy-h_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/acpclient.h \
    ../src/config.h \
    ../src/ecosystem.h \
    ../src/harnessserver.h \
    ../src/mcpmanager.h \
    ../src/messagemodel.h \
    ../src/terminalsession.h \
    ../src/theme.h \
    ../src/workspacemodel.h \
    ../src/translator.h

LIBS += -lutil

SOURCES += \
    deepcrasy-h_tests.cpp \
    ../src/acpclient.cpp \
    ../src/config.cpp \
    ../src/ecosystem.cpp \
    ../src/gitstatus.cpp \
    ../src/terminalsession.cpp \
    ../src/harnessserver.cpp \
    ../src/mcpmanager.cpp \
    ../src/messagemodel.cpp \
    ../src/theme.cpp \
    ../src/workspacemodel.cpp \
    ../src/translator.cpp
