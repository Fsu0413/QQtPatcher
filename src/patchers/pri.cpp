// SPDX-License-Identifier: Unlicense

#include "argument.h"
#include "log.h"
#include "patch.h"
#include <QDir>

class PriPatcher : public Patcher
{
    Q_OBJECT

public:
    PriPatcher(const QString &crossMkspecStartsWith, const QStringList &fileNames);
    ~PriPatcher() override;

    QStringList findFileToPatch() const override;
    void openSSLDirWarning(const QString &file) const;

    virtual bool shouldPatch(const QString &file) const = 0;

protected:
    const QString crossMkspecStartsWith;
    const QStringList fileNames;

    static bool opensslDirWarningDone;
};

bool PriPatcher::opensslDirWarningDone = false;

PriPatcher::PriPatcher(const QString &crossMkspecStartsWith, const QStringList &fileNames)
    : crossMkspecStartsWith(crossMkspecStartsWith)
    , fileNames(fileNames)
{
}

PriPatcher::~PriPatcher()
{
}

QStringList PriPatcher::findFileToPatch() const
{
    if (ArgumentsAndSettings::qtQVersion().majorVersion() != 5)
        return QStringList();

    // Output a warning when a linked OpenSSL is found
    // may need patch manually when OpenSSL build dir moved
    if (!opensslDirWarningDone && QDir(ArgumentsAndSettings::qtDir()).exists(QStringLiteral("mkspecs/modules/qt_lib_network_private.pri")))
        openSSLDirWarning(QStringLiteral("mkspecs/modules/qt_lib_network_private.pri"));

    // Seems Qt 5.12 needs to do such patch
    // Qt 5.9 does not have these stuff
    // I have not built Qt 5.10/5.11, so I can't confirm
    // All of my builds of Qt 5.13 have been removed, I can't confirm either
    // Qt 5.14 has this problem fixed(Since QQtPatcher won't support Qt 5.14, I will not test)
    if (ArgumentsAndSettings::qtQVersion().minorVersion() >= 10 && ArgumentsAndSettings::qtQVersion().minorVersion() <= 13) {
        if (ArgumentsAndSettings::crossMkspec().startsWith(crossMkspecStartsWith)) {
            QStringList r;
            foreach (const QString &fileName, fileNames) {
                if (QDir(ArgumentsAndSettings::qtDir()).exists(fileName) && shouldPatch(fileName))
                    r << fileName;
            }

            return r;
        }
    }
    return QStringList();

    // original QtBinPatcher patches all .pri files, but I don't know why
}

void PriPatcher::openSSLDirWarning(const QString &file) const
{
    if (file.contains(QStringLiteral("qt_lib_network_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    QString value = l.mid(equalMark + 1).trimmed();
                    if (key == QStringLiteral("QMAKE_LIBS_OPENSSL") && !value.isEmpty()) {
                        QBPLOGW(QString(QStringLiteral("Warning: Seems like you are using linked OpenSSL. "
                                                       "Since we can\'t detect the path where you put OpenSSL in, "
                                                       "you should probably manually modify %1 after you moved OpenSSL."))
                                    .arg(f.fileName()));
                        opensslDirWarningDone = true;
                        return;
                    }
                }
            }
            f.close();
        }
    }
}

class PriPatcherAndroid : public PriPatcher
{
    Q_OBJECT

public:
    Q_INVOKABLE PriPatcherAndroid();
    ~PriPatcherAndroid() override;

    bool shouldPatch(const QString &file) const override;
    bool patchFile(const QString &file) const override;
};

PriPatcherAndroid::PriPatcherAndroid()
    : PriPatcher(QStringLiteral("android-"), {QStringLiteral("mkspecs/modules/qt_lib_gui_private.pri")})
{
}

PriPatcherAndroid::~PriPatcherAndroid()
{
}

bool PriPatcherAndroid::shouldPatch(const QString &file) const
{
    if (file.contains(QStringLiteral("qt_lib_gui_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    QString value = l.mid(equalMark + 1).trimmed();
                    if ((key == QStringLiteral("QMAKE_LIBS_OPENGL_ES2") || key == QStringLiteral("QMAKE_LIBS_EGL")) && !value.startsWith(QStringLiteral("-l"))) {
                        f.close();
                        return true;
                    }
                }
            }
            f.close();
        }
    } else
        return false;

    return false;
}

bool PriPatcherAndroid::patchFile(const QString &file) const
{
    if (file.contains(QStringLiteral("qt_lib_gui_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray toWrite;
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    QString value = l.mid(equalMark + 1).trimmed();
                    if (key == QStringLiteral("QMAKE_LIBS_OPENGL_ES2")) {
                        value = QStringLiteral("-lGLESv2");
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    } else if (key == QStringLiteral("QMAKE_LIBS_EGL")) {
                        value = QStringLiteral("-lEGL");
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    }
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

class PriPatcherWin32 : public PriPatcher
{
    Q_OBJECT

public:
    Q_INVOKABLE PriPatcherWin32();
    ~PriPatcherWin32() override;

    bool shouldPatch(const QString &file) const override;
    bool patchFile(const QString &file) const override;

    QString addPrefixSuffix(const QString &libName) const;
};

PriPatcherWin32::PriPatcherWin32()
    : PriPatcher(QStringLiteral("win32-"),
                 {QStringLiteral("mkspecs/modules/qt_lib_gui_private.pri"), QStringLiteral("mkspecs/modules/qt_lib_network_private.pri"),
                  QStringLiteral("mkspecs/modules/qt_lib_multimedia_private.pri")})
{
}

PriPatcherWin32::~PriPatcherWin32()
{
}

bool PriPatcherWin32::shouldPatch(const QString &file) const
{ // win32-msvc and win32-g++ use different grammar. MSVC does not use -l
    if (file.contains(QStringLiteral("qt_lib_network_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    if (key == QStringLiteral("QMAKE_LIBS_NETWORK")) {
                        f.close();
                        return true;
                    }
                }
            }
            f.close();
        }
    } else if (file.contains(QStringLiteral("qt_lib_gui_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    if ((key == QStringLiteral("QMAKE_LIBS_DXGUID")) || (key == QStringLiteral("QMAKE_LIBS_D3D9")) || (key == QStringLiteral("QMAKE_LIBS_DXGI"))
                        || (key == QStringLiteral("QMAKE_LIBS_D3D11")) || (key == QStringLiteral("QMAKE_LIBS_D2D1")) || (key == QStringLiteral("QMAKE_LIBS_D2D1_1"))
                        || (key == QStringLiteral("QMAKE_LIBS_DXGI1_2")) || (key == QStringLiteral("QMAKE_LIBS_D3D11_1")) || (key == QStringLiteral("QMAKE_LIBS_DWRITE"))
                        || (key == QStringLiteral("QMAKE_LIBS_DWRITE_1")) || (key == QStringLiteral("QMAKE_LIBS_DWRITE_2"))) {
                        f.close();
                        return true;
                    }
                }
            }
            f.close();
        }
    } else if (file.contains(QStringLiteral("qt_lib_multimedia_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    if ((key == QStringLiteral("QMAKE_LIBS_WMF")) || (key == QStringLiteral("QMAKE_LIBS_DIRECTSHOW"))) {
                        f.close();
                        return true;
                    }
                }
            }
            f.close();
        }
    } else
        return false;

    return false;
}

bool PriPatcherWin32::patchFile(const QString &file) const
{
    if (file.contains(QStringLiteral("qt_lib_network_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray toWrite;
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    QString value = l.mid(equalMark + 1).trimmed();
                    if (key == QStringLiteral("QMAKE_LIBS_NETWORK")) {
                        value = addPrefixSuffix(QStringLiteral("ws2_32"));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    }
                }
                toWrite.append(arr);
            }
            f.close();

            f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
            f.write(toWrite);
            f.close();
        } else
            return false;
    } else if (file.contains(QStringLiteral("qt_lib_gui_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray toWrite;
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    QString value = l.mid(equalMark + 1).trimmed();
                    if (key == QStringLiteral("QMAKE_LIBS_DXGUID")) {
                        value = addPrefixSuffix(QStringLiteral("dxguid"));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    } else if (key == QStringLiteral("QMAKE_LIBS_D3D9")) {
                        value = addPrefixSuffix(QStringLiteral("d3d9"));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    } else if ((key == QStringLiteral("QMAKE_LIBS_DXGI")) || (key == QStringLiteral("QMAKE_LIBS_DXGI1_2"))) {
                        value = addPrefixSuffix(QStringLiteral("dxgi"));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    } else if ((key == QStringLiteral("QMAKE_LIBS_D3D11")) || (key == QStringLiteral("QMAKE_LIBS_D3D11_1"))) {
                        value = addPrefixSuffix(QStringLiteral("d3d11"));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    } else if ((key == QStringLiteral("QMAKE_LIBS_D2D1")) || (key == QStringLiteral("QMAKE_LIBS_D2D1_1"))) {
                        value = addPrefixSuffix(QStringLiteral("d2d1"));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    } else if ((key == QStringLiteral("QMAKE_LIBS_DWRITE")) || (key == QStringLiteral("QMAKE_LIBS_DWRITE_1")) || (key == QStringLiteral("QMAKE_LIBS_DWRITE_2"))) {
                        value = addPrefixSuffix(QStringLiteral("dwrite"));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    }
                }
                toWrite.append(arr);
            }
            f.close();

            f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
            f.write(toWrite);
            f.close();
        } else
            return false;
    } else if (file.contains(QStringLiteral("qt_lib_multimedia_private"))) {
        QFile f(QDir(ArgumentsAndSettings::qtDir()).absoluteFilePath(file));
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray toWrite;
            char arr[10000];
            while (f.readLine(arr, 9999) > 0) {
                QString l = QString::fromUtf8(arr);
                int equalMark = l.indexOf(QLatin1Char('='));
                if (equalMark != -1) {
                    QString key = l.left(equalMark).trimmed();
                    QString value = l.mid(equalMark + 1).trimmed();
                    if (key == QStringLiteral("QMAKE_LIBS_DIRECTSHOW")) {
                        // clang-format off
                        QStringList values {
                            addPrefixSuffix(QStringLiteral("strmiids")),
                            addPrefixSuffix(QStringLiteral("dmoguids")),
                            addPrefixSuffix(QStringLiteral("uuid")),
                            addPrefixSuffix(QStringLiteral("msdmo")),
                            addPrefixSuffix(QStringLiteral("ole32")),
                            addPrefixSuffix(QStringLiteral("oleaut32")),
                        };
                        // clang-format on
                        value = values.join(QStringLiteral(" "));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    } else if (key == QStringLiteral("QMAKE_LIBS_WMF")) {
                        // clang-format off
                        QStringList values {
                            addPrefixSuffix(QStringLiteral("strmiids")),
                            addPrefixSuffix(QStringLiteral("dmoguids")),
                            addPrefixSuffix(QStringLiteral("uuid")),
                            addPrefixSuffix(QStringLiteral("msdmo")),
                            addPrefixSuffix(QStringLiteral("ole32")),
                            addPrefixSuffix(QStringLiteral("oleaut32")),
                            addPrefixSuffix(QStringLiteral("Mf")),
                            addPrefixSuffix(QStringLiteral("Mfuuid")),
                            addPrefixSuffix(QStringLiteral("Mfplat")),
                            addPrefixSuffix(QStringLiteral("Propsys")),
                        };
                        // clang-format on
                        value = values.join(QStringLiteral(" "));
                        l = key + QStringLiteral(" = ") + value + QStringLiteral("\n");
                        strcpy(arr, l.toUtf8().constData());
                    }
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

QString PriPatcherWin32::addPrefixSuffix(const QString &libName) const
{
    if (ArgumentsAndSettings::crossMkspec().contains(QStringLiteral("msvc")))
        return libName + QStringLiteral(".lib");
    else
        return QStringLiteral("-l") + libName;
}

REGISTER_PATCHER(PriPatcherAndroid)
REGISTER_PATCHER(PriPatcherWin32)

#include "pri.moc"
