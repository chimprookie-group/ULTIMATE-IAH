QT += network core gui testlib
CONFIG += c++17 testcase
TARGET = deepcrasy-h_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/config.h \
    ../src/ecosystem.h \
    ../src/harnessserver.h \
    ../src/harnessconnections.h \
    ../src/nineroutermanager.h \
    ../src/mausbotmanager.h \
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
    ../src/nineroutermanager.cpp \
    ../src/mausbotmanager.cpp \
    ../src/theme.cpp \
    ../src/workspacemodel.cpp \
    ../src/translator.cpp
