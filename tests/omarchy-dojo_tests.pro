QT += core gui qml quick quickcontrols2 testlib
CONFIG += c++17 testcase
TARGET = omarchy-dojo_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/backend.h \
    ../src/catalog.h \
    ../src/combo.h \
    ../src/keygrab.h \
    ../src/store.h \
    ../src/theme.h \
    ../src/translator.h

SOURCES += \
    omarchy-dojo_tests.cpp \
    ../src/backend.cpp \
    ../src/catalog.cpp \
    ../src/combo.cpp \
    ../src/keygrab.cpp \
    ../src/store.cpp \
    ../src/theme.cpp \
    ../src/translator.cpp

RESOURCES += ../src/resources.qrc
