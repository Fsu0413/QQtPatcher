#include "argument.h"
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

    bool shouldPatch(const QString &file) const;
};

PrlPatcher::PrlPatcher()
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

bool PrlPatcher::patchFile(const QString &file) const
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
            QString l = QString::fromUtf8(arr);
            int equalMark = l.indexOf('=');
            if (equalMark != -1) {
                QString key = l.left(equalMark).trimmed();
                QString value = l.mid(equalMark + 1).trimmed();
                if (key == "QMAKE_PRL_LIBS") {
                    QStringList r;
                    QStringList splited = value.split(" ", QString::SkipEmptyParts);
                    foreach (const QString &m, splited) {
                        QString n = m;
                        if (n.startsWith("-L=")) {
                            if (QDir(n.mid(3).replace("\\\\", "\\")) == oldLibDir)
                                n = QStringLiteral("-L=") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                        } else if (n.startsWith("-L")) {
                            if (QDir(n.mid(2).replace("\\\\", "\\")) == oldLibDir)
                                n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
                        } else if (!n.startsWith("-l")) {
                            QFileInfo fi(n.replace("\\\\", "\\"));
                            if (QDir(fi.absolutePath()) == oldLibDir) {
                                QFileInfo fiNew(newLibDir, fi.baseName());
                                n = QDir::fromNativeSeparators(fiNew.absoluteFilePath());
                            }
                        }
                        r << n;
                    }
                    value = r.join(" ");
                    l = QStringLiteral("QMAKE_PRL_LIBS = ") + value + QStringLiteral("\n");
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

    return true;
}

bool PrlPatcher::shouldPatch(const QString &file) const
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
                            if (QDir(QFileInfo(n.replace("\\\\", "\\")).absolutePath()) == oldLibDir) {
                                f.close();
                                return true;
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
