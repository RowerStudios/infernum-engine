QT += widgets core

CONFIG += c++17

TARGET = infernum_editor
TEMPLATE = app
RESOURCES += icons.qrc

SOURCES += \
    src/ui_element.cpp \
    src/viewport_widget.cpp \
    src/property_editor.cpp \
    src/code_editor.cpp \
    src/main_window.cpp \
    src/main.cpp \
    ../src/cpp/engine.cpp \
    ../src/cpp/tween.cpp \
    ../src/cpp/ui.cpp \
    ../src/cpp/sounds.cpp

HEADERS += \
    src/ui_element.h \
    src/viewport_widget.h \
    src/property_editor.h \
    src/code_editor.h \
    src/main_window.h

INCLUDEPATH += \
    /home/rower/Data/infernum-engine/editor \
    /home/rower/Data/infernum-engine/src/header \
    /usr/include/SDL3

QMAKE_CXXFLAGS += -std=gnu++23

LIBS += -lSDL3 -lSDL3_ttf -lSDL3_image -ljsoncpp
