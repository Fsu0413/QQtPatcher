#include "argument.h"
#include "patch.h"
#include <QDir>

class LaPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE LaPatcher();
    ~LaPatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;

    bool shouldPatch(const QString &file) const;
};

LaPatcher::LaPatcher()
{
}

LaPatcher::~LaPatcher()
{
}

QStringList LaPatcher::findFileToPatch() const
{
    // Windows version of Qt4 doesn't support libtool, temporily kill it
    if (ArgumentsAndSettings::qtQVersion().majorVersion() != 5)
        return QStringList();

    // patch lib/libQt5*.la if libtool is enabled, otherwise patch nothing
    // don't know whether MinGW versions supports libtool, need research.
    // Assuming no support for libtool in MinGW version for now

    if (!ArgumentsAndSettings::crossMkspec().startsWith("win")) {
        QDir libDir(ArgumentsAndSettings::qtDir());
        if (!libDir.cd("lib"))
            return QStringList();

        libDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
        libDir.setNameFilters({"libQt5*.la", "libEnginio.la"});
        QStringList r;
        QStringList l = libDir.entryList();
        foreach (const QString &f, l) {
            if (shouldPatch(f))
                r << (QStringLiteral("lib/") + f);
        }
        return r;
    }

    return QStringList();
}

bool LaPatcher::patchFile(const QString &file) const
{
    QDir qtDir(ArgumentsAndSettings::qtDir());

    QDir oldLibDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/lib"));
    QDir newLibDir(ArgumentsAndSettings::newDir() + QStringLiteral("/lib"));

    // It is assumed that no spaces is in the olddir prefix
    QFile f(qtDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray toWrite;
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString str = QString::fromUtf8(arr);
            str = str.trimmed();
            if (str.startsWith("dependency_libs=")) {
                str = str.mid(17);
                str.chop(1);
                QStringList l = str.split(" ", QString::SkipEmptyParts);
                QStringList r;
                foreach (const QString &m, l) {
                    QString n = m;
                    if (n.startsWith("-L=")) {
                        if (QDir(n.mid(3).replace("\\\\", "\\")) == oldLibDir)
                            n = QStringLiteral("-L=") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                    } else if (n.startsWith("-L")) {
                        if (QDir(n.mid(2).replace("\\\\", "\\")) == oldLibDir)
                            n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                    } else if (!n.startsWith("-l")) {
                        QFileInfo fi(QString(n).replace("\\\\", "\\"));
                        if (QDir(fi.absolutePath()) == oldLibDir) {
                            QFileInfo fiNew(newLibDir, fi.baseName());
                            n = QDir::fromNativeSeparators(fiNew.absoluteFilePath());
                        }
                    }
                    r << n;
                }
                str = QStringLiteral("dependency_libs=\'") + r.join(' ') + "\'\n";
                strcpy(arr, str.toUtf8().constData());
            } else if (str.startsWith("libdir=")) {
                str = str.mid(8);
                str.chop(1);

                QString equalMark;
                if (str.startsWith("=")) {
                    equalMark = QStringLiteral("=");
                    str = str.mid(1);
                }

                if (QDir(QString(str).replace("\\\\", "\\")) == oldLibDir) {
                    str = QStringLiteral("libdir=\'") + equalMark + QDir::toNativeSeparators(newLibDir.absolutePath()).replace("\\", "\\\\") + QStringLiteral("\'\n");
                    strcpy(arr, str.toUtf8().constData());
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

bool LaPatcher::shouldPatch(const QString &file) const
{
    QDir libDir(ArgumentsAndSettings::qtDir());
    if (!libDir.cd("lib"))
        return false;

    QDir oldLibDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/lib"));

    // it is assumed that no spaces is in the olddir prefix

    QFile f(libDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString str = QString::fromUtf8(arr);
            str = str.trimmed();
            if (str.startsWith("dependency_libs=")) {
                str = str.mid(17);
                str.chop(1);
                QStringList l = str.split(" ", QString::SkipEmptyParts);
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
                    }
                }
            } else if (str.startsWith("libdir=")) {
                str = str.mid(8);
                str.chop(1);
                if (str.startsWith("="))
                    str = str.mid(1);

                if (QDir(QString(str).replace("\\\\", "\\")) == oldLibDir) {
                    f.close();
                    return true;
                }
            }
        }
        f.close();
    }

    return false;
}

REGISTER_PATCHER(LaPatcher)

#include "la.moc"
