# SPDX-License-Identifier: Unlicense

QT -= gui

lessThan(QT_MAJOR_VERSION, 5) {
    error("QQtPatcher requires Qt after 5.6.")
}

equals(QT_MAJOR_VERSION, 5): lessThan(QT_MINOR_VERSION, 6) {
    error("QQtPatcher requires Qt after 5.6.")
}

CONFIG += c++11 console
CONFIG -= app_bundle
TARGET = QQtPatcher

VERSION = 0.8.2

win32 {
    RC_ICONS = res/QQtPatcher.ico
    QMAKE_TARGET_PRODUCT = "QQtPatcher"
    QMAKE_TARGET_DESCRIPTION = "Tool for patching paths in compiled Qt library"
    # QMAKE_TARGET_COMPANY = "Mogara.org"
    QMAKE_TARGET_COPYRIGHT = "Frank Su, 2019-2022. https://build-qt.fsu0413.me"
}

DEFINES += QT_DEPRECATED_WARNINGS QT_DISABLE_DEPRECATED_BEFORE=0x070000 VERSION=\\\"$$VERSION\\\" QT_NO_CAST_FROM_ASCII

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

# workaround Qt 6 qmake which don't add following libraries during qmake
equals(QT_MAJOR_VERSION, 6): msvc {
	LIBS += -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32
}
