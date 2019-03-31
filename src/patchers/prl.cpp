#include "argument.h"
#include "log.h"
#include "patch.h"
#include <QDir>

class PrlPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE PrlPatcher();
    ~PrlPatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;

    QString patchQmakePrlLibs(const QDir &oldLibDir, const QDir &newLibDir, const QString &value) const;

    bool shouldPatch(const QString &file) const;

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

QStringList PrlPatcher::findFileToPatch() const
{
    // patch lib/*.prl, no exception.

    QDir libDir(ArgumentsAndSettings::qtDir());
    if (!libDir.cd("lib"))
        return QStringList();

    libDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
    libDir.setNameFilters({"*.prl"});
    QStringList r;
    QStringList l = libDir.entryList();
    foreach (const QString &f, l) {
        if (shouldPatch(f))
            r << (QStringLiteral("lib/") + f);
    }
    return r;
}

QString PrlPatcher::patchQmakePrlLibs(const QDir &oldLibDir, const QDir &newLibDir, const QString &value) const
{
    QStringList r;
    QStringList splited = value.split(" ", QString::SkipEmptyParts);

    static QDir buildLibDir(ArgumentsAndSettings::buildDir() + QStringLiteral("/lib"));

    foreach (const QString &m, splited) {
        QString n = m;
        if (n.startsWith("-L=")) {
            if (QDir(n.mid(3).replace("\\\\", "\\")) == oldLibDir)
                n = QStringLiteral("-L=") + QDir::fromNativeSeparators(newLibDir.absolutePath());
            else {
                if (!ArgumentsAndSettings::buildDir().isEmpty() && ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                    if (QDir(n.mid(3).replace("\\\\", "\\")) == buildLibDir)
                        n = QStringLiteral("-L=") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                }
            }
        } else if (n.startsWith("-L")) {
            if (QDir(n.mid(2).replace("\\\\", "\\")) == oldLibDir)
                n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
            else {
                if (!ArgumentsAndSettings::buildDir().isEmpty() && ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                    if (QDir(n.mid(2).replace("\\\\", "\\")) == buildLibDir)
                        n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                }
            }
        } else if (!n.startsWith("-l")) {
            QFileInfo fi(QString(n).replace("\\\\", "\\"));

            if (fi.isAbsolute() && QDir(fi.absolutePath()) == oldLibDir) {
                QFileInfo fiNew(newLibDir, fi.fileName());
                if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
                    n = QDir::fromNativeSeparators(fiNew.absoluteFilePath());
                else
                    n = QDir::toNativeSeparators(fiNew.absoluteFilePath()).replace("\\", "\\\\");
            } else {
                if (!ArgumentsAndSettings::buildDir().isEmpty() && ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                    if (fi.isAbsolute() && QDir(fi.absolutePath()) == buildLibDir) {
                        QFileInfo fiNew(newLibDir, fi.fileName());
                        n = QDir::toNativeSeparators(fiNew.absoluteFilePath()).replace("\\", "\\\\");
                    }
                }
            }
        }
        r << n;
    }
    return r.join(" ");
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
            int equalMark = l.indexOf('=');
            if (equalMark != -1) {
                QString key = l.left(equalMark).trimmed();
                QString value = l.mid(equalMark + 1).trimmed();
                if (key == "QMAKE_PRL_LIBS") {
                    value = patchQmakePrlLibs(oldLibDir, newLibDir, value);
                    l = QStringLiteral("QMAKE_PRL_LIBS = ") + value + QStringLiteral("\n");
                    strcpy(arr, l.toUtf8().constData());
                } else if (key == "QMAKE_PRL_BUILD_DIR") {
                    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                        QString rp = oldDir.relativeFilePath(value);
                        if (rp.contains("..")) {
                            if (!ArgumentsAndSettings::buildDir().isEmpty())
                                rp = buildDir.relativeFilePath(value);
                        }

                        if (!rp.contains("..")) {
                            value = QDir::fromNativeSeparators(QDir::cleanPath(newDir.absolutePath() + "/" + rp));
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
    if (!libDir.cd("lib"))
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

    QFile f(libDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString l = QString::fromUtf8(arr);
            int equalMark = l.indexOf('=');
            if (equalMark != -1) {
                QString key = l.left(equalMark).trimmed();
                QString value = l.mid(equalMark + 1).trimmed();
                if (key == "QMAKE_PRL_LIBS") {
                    QStringList l = value.split(" ", QString::SkipEmptyParts);
                    foreach (const QString &m, l) {
                        QString n = m;
                        if (n.startsWith("-L=")) {
                            if (QDir(n.mid(3).replace("\\\\", "\\")) == oldLibDir) {
                                f.close();
                                return true;
                            }
                        } else if (n.startsWith("-L")) {
                            if (QDir(n.mid(2).replace("\\\\", "\\")) == oldLibDir) {
                                f.close();
                                return true;
                            }
                        } else if (!n.startsWith("-l")) {
                            if (QDir(QFileInfo(QString(n).replace("\\\\", "\\")).absolutePath()) == oldLibDir) {
                                f.close();
                                return true;
                            }
                        } else if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4 && !ArgumentsAndSettings::buildDir().isEmpty()) {
                            if (n.startsWith("-L=")) {
                                if (QDir(n.mid(3).replace("\\\\", "\\")) == buildLibDir) {
                                    f.close();
                                    return true;
                                }
                            } else if (n.startsWith("-L")) {
                                if (QDir(n.mid(2).replace("\\\\", "\\")) == buildLibDir) {
                                    f.close();
                                    return true;
                                }
                            } else if (!n.startsWith("-l")) {
                                if (QDir(QFileInfo(QString(n).replace("\\\\", "\\")).absolutePath()) == buildLibDir) {
                                    f.close();
                                    return true;
                                }
                            }
                        }
                    }
                } else if (key == "QMAKE_PRL_BUILD_DIR") {
                    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                        if (!oldDir.relativeFilePath(value).contains("..")) {
                            f.close();
                            return true;
                        } else if (!ArgumentsAndSettings::buildDir().isEmpty()) {
                            if (!buildDir.relativeFilePath(value).contains("..")) {
                                f.close();
                                return true;
                            }
                        } else {
                            if (!qt4NoBuildDirWarn) {
                                qt4NoBuildDirWarn = true;
                                QBPLOGW("Your build of Qt seems just compiled, due to bug in Qt compile system, you should provide a config file which provides a build-dir.");
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

REGISTER_PATCHER(PrlPatcher)

#include "prl.moc"
