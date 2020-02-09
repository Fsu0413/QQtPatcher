import qbs 1.0
import qbs.Utilities

Project {
    name: "QQtPatcherProject"
    minimumQbsVersion: "1.11"

    QtApplication {
        name: "QQtPatcher"

        Group {
            condition: {
                if (Utilities.versionCompare(Qt.core.version, "5.6.0") < 0)
                    throw "QQtPatcher requires Qt 5 after 5.6.";

                return true;
            }
            files: [
                "src/log.cpp",
                "src/main.cpp",
                "src/patch.cpp",
                "src/argument.cpp",
                "src/backup.cpp",
                "src/patchers/pri.cpp",
                "src/patchers/prl.cpp",
                "src/patchers/qmakeconf.cpp",
                "src/patchers/qtconf.cpp",
                "src/patchers/binary.cpp",
                "src/patchers/cmake.cpp",
                "src/patchers/la.cpp",
                "src/patchers/pc.cpp",

                "src/argument.h",
                "src/backup.h",
                "src/log.h",
                "src/patch.h",
            ]
        }

        cpp.includePaths: ["src"]

        version: "0.7.0"
        consoleApplication: true
        cpp.defines: [
            "QT_DEPRECATED_WARNINGS",
            "QT_DISABLE_DEPRECATED_BEFORE=0x060000",
            "VERSION=\"" + version + "\"",
            "QT_NO_CAST_FROM_ASCII"
        ]
        cpp.cxxLanguageVersion: ["c++11"]
        cpp.useLanguageVersionFallback: true
        cpp.minimumWindowsVersion: "6.1"
        cpp.windowsApiFamily: "desktop"
    }
}
