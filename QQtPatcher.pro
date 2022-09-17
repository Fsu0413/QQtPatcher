# SPDX-License-Identifier: Unlicense

QT -= gui

!equals(QT_MAJOR_VERSION, "5") {
    error("QQtPatcher requires Qt 5 after 5.6.")
}

lessThan(QT_MINOR_VERSION, 6) {
    error("QQtPatcher requires Qt 5 after 5.6.")
}

CONFIG += c++11 console
CONFIG -= app_bundle
TARGET = QQtPatcher

VERSION = 0.8.1

win32 {
    RC_ICONS = res/QQtPatcher.ico
    QMAKE_TARGET_PRODUCT = "QQtPatcher"
    QMAKE_TARGET_DESCRIPTION = "Tool for patching paths in compiled Qt library"
    # QMAKE_TARGET_COMPANY = "Mogara.org"
    QMAKE_TARGET_COPYRIGHT = "Frank Su, 2019-2022. https://build-qt.fsu0413.me"
}

DEFINES += QT_DEPRECATED_WARNINGS QT_DISABLE_DEPRECATED_BEFORE=0x060000 VERSION=\\\"$$VERSION\\\" QT_NO_CAST_FROM_ASCII

SOURCES += \
        src/main.cpp \
        src/log.cpp \
        src/argument.cpp \
        src/backup.cpp \
        src/patch.cpp \
        src/patchers/binary.cpp \
        src/patchers/cmake.cpp \
        src/patchers/la.cpp \
        src/patchers/pc.cpp \
        src/patchers/pri.cpp \
        src/patchers/prl.cpp \
        src/patchers/qtconf.cpp \
        src/patchers/qmakeconf.cpp

HEADERS += \
        src/log.h \
        src/argument.h \
        src/backup.h \
        src/patch.h

INCLUDEPATH += src
