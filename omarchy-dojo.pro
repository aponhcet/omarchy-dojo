QT += core gui qml quick quickcontrols2

CONFIG += c++17 release
TARGET = omarchy-dojo
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/catalog.h \
    src/combo.h \
    src/keygrab.h \
    src/store.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/catalog.cpp \
    src/combo.cpp \
    src/keygrab.cpp \
    src/store.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
