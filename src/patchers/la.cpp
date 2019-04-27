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
    // MinGW don't support libtool

    if (!ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("win"))) {
        QDir libDir(ArgumentsAndSettings::qtDir());
        if (!libDir.cd(QStringLiteral("lib")))
            return QStringList();

        libDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
        if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
            libDir.setNameFilters({QStringLiteral("libQt5*.la"), QStringLiteral("libEnginio.la")});
        else
            libDir.setNameFilters({QStringLiteral("libQt*.la"), QStringLiteral("libphonon.la")});
        QStringList r;
        QStringList l = libDir.entryList();
        foreach (const QString &f, l) {
            if ((ArgumentsAndSettings::qtQVersion().majorVersion() == 4) && f.startsWith(QStringLiteral("libQt5")))
                continue;

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
            if (str.startsWith(QStringLiteral("dependency_libs="))) {
                str = str.mid(17);
                str.chop(1);
                QStringList l = str.split(QStringLiteral(" "), QString::SkipEmptyParts);
                QStringList r;
                foreach (const QString &m, l) {
                    QString n = m;
                    if (n.startsWith(QStringLiteral("-L="))) {
                        if (QDir(n.mid(3).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir)
                            n = QStringLiteral("-L=") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                    } else if (n.startsWith(QStringLiteral("-L"))) {
                        if (QDir(n.mid(2).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir)
                            n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                    } else if (!n.startsWith(QStringLiteral("-l"))) {
                        QFileInfo fi(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\")));
                        if (QDir(fi.absolutePath()) == oldLibDir) {
                            QFileInfo fiNew(newLibDir, fi.baseName());
                            n = QDir::fromNativeSeparators(fiNew.absoluteFilePath());
                        }
                    }
                    r << n;
                }
                str = QStringLiteral("dependency_libs=\'") + r.join(QLatin1Char(' ')) + QStringLiteral("\'\n");
                strcpy(arr, str.toUtf8().constData());
            } else if (str.startsWith(QStringLiteral("libdir="))) {
                str = str.mid(8);
                str.chop(1);

                QString equalMark;
                if (str.startsWith(QStringLiteral("="))) {
                    equalMark = QStringLiteral("=");
                    str = str.mid(1);
                }

                if (QDir(QString(str).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir) {
                    str = QStringLiteral("libdir=\'") + equalMark + QDir::toNativeSeparators(newLibDir.absolutePath()).replace(QStringLiteral("\\"), QStringLiteral("\\\\"))
                        + QStringLiteral("\'\n");
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
    if (!libDir.cd(QStringLiteral("lib")))
        return false;

    QDir oldLibDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/lib"));

    // it is assumed that no spaces is in the olddir prefix

    QFile f(libDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString str = QString::fromUtf8(arr);
            str = str.trimmed();
            if (str.startsWith(QStringLiteral("dependency_libs="))) {
                str = str.mid(17);
                str.chop(1);
                QStringList l = str.split(QStringLiteral(" "), QString::SkipEmptyParts);
                foreach (const QString &m, l) {
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
                        if (QDir(QFileInfo(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))).absolutePath()) == oldLibDir) {
                            f.close();
                            return true;
                        }
                    }
                }
            } else if (str.startsWith(QStringLiteral("libdir="))) {
                str = str.mid(8);
                str.chop(1);
                if (str.startsWith(QStringLiteral("=")))
                    str = str.mid(1);

                if (QDir(QString(str).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir) {
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
