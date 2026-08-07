QT += core gui widgets multimedia

CONFIG += c++17

TARGET = snake_game
TEMPLATE = app

DEFINES += PROJECT_ROOT=\\\"$$PWD\\\"

SOURCES += \
    src/app_paths.cpp \
    src/audio_manager.cpp \
    src/block_title_widget.cpp \
    src/common.cpp \
    src/game_screen.cpp \
    src/leaderboard_screen.cpp \
    src/main.cpp \
    src/main_window.cpp \
    src/menu_screen.cpp \
    src/settings_screen.cpp \
    src/storage.cpp

HEADERS += \
    src/app_paths.h \
    src/audio_manager.h \
    src/block_title_widget.h \
    src/common.h \
    src/game_screen.h \
    src/leaderboard_screen.h \
    src/main_window.h \
    src/menu_screen.h \
    src/settings_screen.h \
    src/storage.h
