// SPDX-License-Identifier: Unlicense

#include "argument.h"
#include "log.h"
#include "patch.h"

#include <QDir>
#include <QRegularExpression>

class PrlPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE PrlPatcher();
    ~PrlPatcher() override;

    QStringList findFileToPatchInternal(const QDir &dir, bool recursive = true) const;
    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;

    QString patchQmakePrlLibs(const QDir &oldLibDir, const QDir &newLibDir, const QString &value) const;

    bool shouldPatch(const QString &file) const;

    QString win32AddPrefixSuffix(const QString &libName) const;

private:
    mutable bool qt4NoBuildDirWarn;
};

PrlPatcher::PrlPatcher()
    : qt4NoBuildDirWarn(false)
{
}

PrlPatcher::~PrlPatcher()
{
}

QStringList PrlPatcher::findFileToPatchInternal(const QDir &dir, bool recursive) const
{
    QDir qtDir(ArgumentsAndSettings::qtDir());
    QDir libDir(dir);

    libDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
    libDir.setNameFilters({QStringLiteral("*.prl")});
    QStringList r;
    QStringList l = libDir.entryList();
    foreach (const QString &f, l) {
        if (shouldPatch(dir.absoluteFilePath(f)))
            r << qtDir.relativeFilePath(dir.absolutePath()) + QStringLiteral("/") + f;
    }

    if (recursive) {
        libDir.setFilter(QDir::Dirs | QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot /* | QDir::Executable */);
        libDir.setNameFilters({});
        QStringList l = libDir.entryList();
        foreach (const QString &f, l) {
            QDir subdir(libDir);
            subdir.cd(f);
            r.append(findFileToPatchInternal(subdir, true));
        }
    }

    return r;
}

QStringList PrlPatcher::findFileToPatch() const
{
    // patch **.prl

    QStringList ret;

    QDir libDir(ArgumentsAndSettings::qtDir());
    if (libDir.cd(QStringLiteral("lib")))
        ret.append(findFileToPatchInternal(libDir, false));

    QDir qmlDir(ArgumentsAndSettings::qtDir());
    if (qmlDir.cd(QStringLiteral("qml")))
        ret.append(findFileToPatchInternal(qmlDir, true));

    QDir pluginDir(ArgumentsAndSettings::qtDir());
    if (pluginDir.cd(QStringLiteral("plugins")))
        ret.append(findFileToPatchInternal(pluginDir, true));

    return ret;
}

QString PrlPatcher::patchQmakePrlLibs(const QDir &oldLibDir, const QDir &newLibDir, const QString &value) const
{
    QStringList r;
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList _splitted = value.split(QStringLiteral(" "), QString::KeepEmptyParts);
#else
    QStringList _splitted = value.split(QStringLiteral(" "), Qt::KeepEmptyParts);
#endif

    QStringList splitted;
    QString temp;
    bool flag = false;
    foreach (const QString &_split, _splitted) {
        if (!flag) {
            if (!_split.startsWith(QLatin1Char('"')))
                splitted << _split;
            else {
                flag = true;
                temp = _split.mid(1);
            }
        } else {
            temp.append(QLatin1Char(' ')).append(_split);
            if (_split.endsWith(QLatin1Char('"'))) {
                flag = false;
                splitted << temp.left(temp.length() - 1);
                temp = QString();
            }
        }
    }

    static QDir buildLibDir(ArgumentsAndSettings::buildDir() + QStringLiteral("/lib"));

    foreach (const QString &m, splitted) {
        QString n = m;
        if (n.startsWith(QStringLiteral("-L="))) {
            if (QDir(n.mid(3).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir)
                n = QStringLiteral("-L=") + QDir::fromNativeSeparators(newLibDir.absolutePath());
            else {
                if (!ArgumentsAndSettings::buildDir().isEmpty() && ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                    if (QDir(n.mid(3).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == buildLibDir)
                        n = QStringLiteral("-L=") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                }
            }
        } else if (n.startsWith(QStringLiteral("-L"))) {
            if (QDir(n.mid(2).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir)
                n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
            else {
                if (!ArgumentsAndSettings::buildDir().isEmpty() && ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                    if (QDir(n.mid(2).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == buildLibDir)
                        n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                }
            }
        } else if (!n.startsWith(QStringLiteral("-l"))) {
            QFileInfo fi(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\")));

            if (fi.isAbsolute()) {
                if (QDir(fi.absolutePath()) == oldLibDir) {
                    QFileInfo fiNew(newLibDir, fi.fileName());
                    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
                        n = QDir::fromNativeSeparators(fiNew.absoluteFilePath());
                    else
                        n = QDir::toNativeSeparators(fiNew.absoluteFilePath()).replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
                } else {
                    if ((ArgumentsAndSettings::qtQVersion().majorVersion() == 5 && ArgumentsAndSettings::qtQVersion().minorVersion() >= 10
                         && ArgumentsAndSettings::qtQVersion().minorVersion() <= 13)
                        && ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("win32-"))) {
                        // clang-format off
                        static QStringList knownLists {
                            // libs
                            QStringLiteral("d2d1"),
                            QStringLiteral("d3d9"),
                            QStringLiteral("dwrite"),
                            QStringLiteral("dxguid"),
                            QStringLiteral("advapi32"),
                            QStringLiteral("comdlg32"),
                            QStringLiteral("crypt32"),
                            QStringLiteral("dnsapi"),
                            QStringLiteral("dwmapi"),
                            QStringLiteral("gdi32"),
                            QStringLiteral("iphlpapi"),
                            QStringLiteral("kernel32"),
                            QStringLiteral("mpr"),
                            QStringLiteral("netapi32"),
                            QStringLiteral("ole32"),
                            QStringLiteral("oleaut32"),
                            QStringLiteral("setupapi"),
                            QStringLiteral("shell32"),
                            QStringLiteral("shlwapi"),
                            QStringLiteral("user32"),
                            QStringLiteral("userenv"),
                            QStringLiteral("uuid"),
                            QStringLiteral("uxtheme"),
                            QStringLiteral("version"),
                            QStringLiteral("winmm"),
                            QStringLiteral("winspool"),
                            QStringLiteral("ws2_32"),

                            // plugins
                            QStringLiteral("odbc32"),
                            QStringLiteral("strmiids"),
                            QStringLiteral("mf"),
                            QStringLiteral("mfplat"),
                            QStringLiteral("dxva2"),
                            QStringLiteral("evr"),
                            QStringLiteral("dmoguids"),
                            QStringLiteral("msdmo"),
                            QStringLiteral("propsys"),
                            QStringLiteral("imm32"),
                            QStringLiteral("wtsapi32"),
                            QStringLiteral("d3d11"),
                            QStringLiteral("dxgi"),
                            QStringLiteral("d3d12"),
                            QStringLiteral("d3dcompiler"),
                            QStringLiteral("dcomp"),
                        };
                        // clang-format on

                        QString baseName = QFileInfo(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))).baseName().toLower();
                        foreach (const QString &known, knownLists) {
                            if (baseName.contains(known))
                                n = win32AddPrefixSuffix(known);
                        }
                    }
                }
            } else {
                if (!ArgumentsAndSettings::buildDir().isEmpty() && ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                    if (fi.isAbsolute() && QDir(fi.absolutePath()) == buildLibDir) {
                        QFileInfo fiNew(newLibDir, fi.fileName());
                        n = QDir::toNativeSeparators(fiNew.absoluteFilePath()).replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
                    }
                }
            }
        }
        if (n.contains(QRegularExpression(QStringLiteral("\\s"))))
            n = QStringLiteral("\"") + n + QStringLiteral("\"");

        r << n;
    }
    return r.join(QStringLiteral(" "));
}

bool PrlPatcher::patchFile(const QString &file) const
{
    QDir qtDir(ArgumentsAndSettings::qtDir());

    QDir oldLibDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/lib"));
    QDir newLibDir(ArgumentsAndSettings::newDir() + QStringLiteral("/lib"));
    QDir newDir(ArgumentsAndSettings::newDir());
    QDir oldDir(ArgumentsAndSettings::oldDir());
    QDir buildDir(ArgumentsAndSettings::buildDir());

    // It is assumed that no spaces is in the olddir prefix
    QFile f(qtDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray toWrite;
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString l = QString::fromUtf8(arr);
            int equalMark = l.indexOf(QLatin1Char('='));
            if (equalMark != -1) {
                QString key = l.left(equalMark).trimmed();
                QString value = l.mid(equalMark + 1).trimmed();
                if (key == QStringLiteral("QMAKE_PRL_LIBS")) {
                    value = patchQmakePrlLibs(oldLibDir, newLibDir, value);
                    l = QStringLiteral("QMAKE_PRL_LIBS = ") + value + QStringLiteral("\n");
                    strcpy(arr, l.toUtf8().constData());
                } else if (key == QStringLiteral("QMAKE_PRL_BUILD_DIR")) {
                    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                        QString rp = oldDir.relativeFilePath(value);
                        if (rp.contains(QStringLiteral(".."))) {
                            if (!ArgumentsAndSettings::buildDir().isEmpty())
                                rp = buildDir.relativeFilePath(value);
                        }

                        if (!rp.contains(QStringLiteral(".."))) {
                            value = QDir::fromNativeSeparators(QDir::cleanPath(newDir.absolutePath() + QStringLiteral("/") + rp));
                            l = QStringLiteral("QMAKE_PRL_BUILD_DIR = ") + value + QStringLiteral("\n");
                            strcpy(arr, l.toUtf8().constData());
                        }
                    }
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

    return true;
}

bool PrlPatcher::shouldPatch(const QString &file) const
{
    QDir libDir(ArgumentsAndSettings::qtDir());
    if (!libDir.cd(QStringLiteral("lib")))
        return false;

    QDir oldLibDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/lib"));
    QDir oldDir(ArgumentsAndSettings::oldDir());
    QDir buildLibDir;
    QDir buildDir;
    if (!ArgumentsAndSettings::buildDir().isEmpty()) {
        buildDir = QDir(ArgumentsAndSettings::buildDir());
        buildLibDir = QDir(ArgumentsAndSettings::buildDir() + QStringLiteral("/lib"));
    }

    // it is assumed that no spaces is in the olddir prefix

    QFile f(file);
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString l = QString::fromUtf8(arr);
            int equalMark = l.indexOf(QLatin1Char('='));
            if (equalMark != -1) {
                QString key = l.left(equalMark).trimmed();
                QString value = l.mid(equalMark + 1).trimmed();
                if (key == QStringLiteral("QMAKE_PRL_LIBS")) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                    QStringList _splitted = value.split(QStringLiteral(" "), QString::KeepEmptyParts);
#else
                    QStringList _splitted = value.split(QStringLiteral(" "), Qt::KeepEmptyParts);
#endif

                    QStringList splitted;
                    QString temp;
                    bool flag = false;
                    foreach (const QString &_split, _splitted) {
                        if (!flag) {
                            if (!_split.startsWith(QLatin1Char('"')))
                                splitted << _split;
                            else {
                                flag = true;
                                temp = _split.mid(1);
                            }
                        } else {
                            temp.append(QLatin1Char(' ')).append(_split);
                            if (_split.endsWith(QLatin1Char('"'))) {
                                flag = false;
                                splitted << temp.left(temp.length() - 1);
                                temp = QString();
                            }
                        }
                    }

                    foreach (const QString &m, splitted) {
                        QString n = m;
                        if (n.startsWith(QStringLiteral("-L="))) {
                            if (QDir(n.mid(3).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir) {
                                f.close();
                                return true;
                            }
                        } else if (n.startsWith(QStringLiteral("-L"))) {
                            if (QDir(n.mid(2).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir) {
                                f.close();
                                return true;
                            }
                        } else if (!n.startsWith(QStringLiteral("-l"))) {
                            // Seems Qt 5.12 needs to do such patch
                            // Qt 5.9 does not have these stuff
                            // I have not built Qt 5.10/5.11, so I can't confirm
                            // All of my builds of Qt 5.13 have been removed, I can't confirm either
                            // Qt 5.14 has this problem fixed(Since QQtPatcher won't support Qt 5.14, I will not test)
                            if ((ArgumentsAndSettings::qtQVersion().majorVersion() == 5 && ArgumentsAndSettings::qtQVersion().minorVersion() >= 10
                                 && ArgumentsAndSettings::qtQVersion().minorVersion() <= 13)
                                && ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("win32-"))) {
                                // clang-format off
                                static QStringList knownLists {
                                    // libs
                                    QStringLiteral("d2d1"),
                                    QStringLiteral("d3d9"),
                                    QStringLiteral("dwrite"),
                                    QStringLiteral("dxguid"),
                                    QStringLiteral("advapi32"),
                                    QStringLiteral("comdlg32"),
                                    QStringLiteral("crypt32"),
                                    QStringLiteral("dnsapi"),
                                    QStringLiteral("dwmapi"),
                                    QStringLiteral("gdi32"),
                                    QStringLiteral("iphlpapi"),
                                    QStringLiteral("kernel32"),
                                    QStringLiteral("mpr"),
                                    QStringLiteral("netapi32"),
                                    QStringLiteral("ole32"),
                                    QStringLiteral("oleaut32"),
                                    QStringLiteral("setupapi"),
                                    QStringLiteral("shell32"),
                                    QStringLiteral("shlwapi"),
                                    QStringLiteral("user32"),
                                    QStringLiteral("userenv"),
                                    QStringLiteral("uuid"),
                                    QStringLiteral("uxtheme"),
                                    QStringLiteral("version"),
                                    QStringLiteral("winmm"),
                                    QStringLiteral("winspool"),
                                    QStringLiteral("ws2_32"),

                                    // plugins
                                    QStringLiteral("odbc32"),
                                    QStringLiteral("strmiids"),
                                    QStringLiteral("mf"),
                                    QStringLiteral("mfplat"),
                                    QStringLiteral("dxva2"),
                                    QStringLiteral("evr"),
                                    QStringLiteral("dmoguids"),
                                    QStringLiteral("msdmo"),
                                    QStringLiteral("propsys"),
                                    QStringLiteral("imm32"),
                                    QStringLiteral("wtsapi32"),
                                    QStringLiteral("d3d11"),
                                    QStringLiteral("dxgi"),
                                    QStringLiteral("d3d12"),
                                    QStringLiteral("d3dcompiler"),
                                    QStringLiteral("dcomp"),
                                };
                                // clang-format on
                                QString baseName = QFileInfo(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))).baseName().toLower();
                                foreach (const QString &known, knownLists) {
                                    if (baseName.contains(known)) {
                                        f.close();
                                        return true;
                                    }
                                }
                            }

                            if (QDir(QFileInfo(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))).absolutePath()) == oldLibDir) {
                                f.close();
                                return true;
                            }
                        } else if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4 && !ArgumentsAndSettings::buildDir().isEmpty()) {
                            if (n.startsWith(QStringLiteral("-L="))) {
                                if (QDir(n.mid(3).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == buildLibDir) {
                                    f.close();
                                    return true;
                                }
                            } else if (n.startsWith(QStringLiteral("-L"))) {
                                if (QDir(n.mid(2).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == buildLibDir) {
                                    f.close();
                                    return true;
                                }
                            } else if (!n.startsWith(QStringLiteral("-l"))) {
                                if (QDir(QFileInfo(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))).absolutePath()) == buildLibDir) {
                                    f.close();
                                    return true;
                                }
                            }
                        }
                    }
                } else if (key == QStringLiteral("QMAKE_PRL_BUILD_DIR")) {
                    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                        if (!oldDir.relativeFilePath(value).contains(QStringLiteral(".."))) {
                            f.close();
                            return true;
                        } else if (!ArgumentsAndSettings::buildDir().isEmpty()) {
                            if (!buildDir.relativeFilePath(value).contains(QStringLiteral(".."))) {
                                f.close();
                                return true;
                            }
                        } else {
                            if (!qt4NoBuildDirWarn) {
                                qt4NoBuildDirWarn = true;
                                QBPLOGW(QStringLiteral(
                                    "Your build of Qt seems just built, due to bug in Qt build system, you should provide a config file which provides a build-dir."));
                            }
                        }
                    }
                }
            }
        }
        f.close();
    }

    return false;
}

QString PrlPatcher::win32AddPrefixSuffix(const QString &libName) const
{
    // win32-msvc and win32-g++ use different grammar. MSVC does not use -l
    if (ArgumentsAndSettings::crossMkspec().contains(QStringLiteral("msvc")))
        return libName + QStringLiteral(".lib");
    else
        return QStringLiteral("-l") + libName;
}

REGISTER_PATCHER(PrlPatcher)

#include "prl.moc"
