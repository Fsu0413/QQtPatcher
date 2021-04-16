// SPDX-License-Identifier: Unlicense

#include "argument.h"
#include "log.h"
#include "patch.h"
#include <QDir>

class CMakePatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE CMakePatcher();
    ~CMakePatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;

    bool shouldPatch(const QString &file) const;
};

CMakePatcher::CMakePatcher()
{
}

CMakePatcher::~CMakePatcher()
{
}

QStringList CMakePatcher::findFileToPatch() const
{
    // Qt4 doesn't support CMake
    if (ArgumentsAndSettings::qtQVersion().majorVersion() != 5)
        return QStringList();

    // patch "lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake" in cross versions? or only for android?
    //    _qt5gui_find_extra_libs(EGL "EGL" "" "")
    //    _qt5gui_find_extra_libs(OPENGL "GLESv2" "" "")
    // no absolute patchs should be found in these variables.
    static QString fileName = QStringLiteral("lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake");

    if (ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("android"))) {
        if (QDir(ArgumentsAndSettings::qtDir()).exists(fileName) && shouldPatch(fileName))
            return {fileName};
    }

    return QStringList();

    // original QtBinPatcher patches lib/cmake/Qt5LinguistTools/Qt5LinguistToolsConfig.cmake, but I don't know why
}

bool CMakePatcher::patchFile(const QString &file) const
{
    if (file.contains(QStringLiteral("Qt5Gui"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray toWrite;
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString str = QString::fromUtf8(arr);
                str = str.trimmed();
                if (str.startsWith(QStringLiteral("_qt5gui_find_extra_libs("))) {
                    str = str.mid(24);
                    str.chop(1);
                    QStringList l = str.split(QStringLiteral(" "));
                    if (l.first() == QStringLiteral("EGL") && l.value(1) != QStringLiteral("\"EGL\""))
                        strcpy(arr, "    _qt5gui_find_extra_libs(EGL \"EGL\" \"\" \"\")\n");
                    else if (l.first() == QStringLiteral("OPENGL") && l.value(1) != QStringLiteral("\"GLESv2\""))
                        strcpy(arr, "    _qt5gui_find_extra_libs(OPENGL \"GLESv2\" \"\" \"\")\n");
                }
                toWrite.append(arr);
            }
            f.close();

            f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
            f.write(toWrite);
            f.close();
        } else
            return false;
    } else
        return false;

    return true;
}

bool CMakePatcher::shouldPatch(const QString &file) const
{
    if (file.contains(QStringLiteral("Qt5Gui"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString str = QString::fromUtf8(arr);
                str = str.trimmed();
                if (str.startsWith(QStringLiteral("_qt5gui_find_extra_libs("))) {
                    str = str.mid(24);
                    str.chop(1);
                    QStringList l = str.split(QStringLiteral(" "));
                    QBPLOGV(l.first() + QStringLiteral(", ") + l.value(1));
                    if (l.first() == QStringLiteral("EGL") && l.value(1) != QStringLiteral("\"EGL\"")) {
                        f.close();
                        return true;
                    } else if (l.first() == QStringLiteral("OPENGL") && l.value(1) != QStringLiteral("\"GLESv2\"")) {
                        f.close();
                        return true;
                    }
                }
            }
            f.close();
        }
    }

    return false;
}

REGISTER_PATCHER(CMakePatcher)

#include "cmake.moc"
