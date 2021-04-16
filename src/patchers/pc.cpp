// SPDX-License-Identifier: Unlicense

#include "argument.h"
#include "patch.h"
#include <QDir>

class PcPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE PcPatcher();
    ~PcPatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;

    bool shouldPatch(const QString &file) const;

    void patchQt5(const QString &str, char *arr, const QDir &newDir) const;
    void patchQt4MinGW(const QString &str, char *arr, const QDir &newDir, const QString &fBaseName) const;
    void patchQt4Unix(const QString &str, char *arr, const QDir &newDir, const QString &fBaseName) const;
};

PcPatcher::PcPatcher()
{
}

PcPatcher::~PcPatcher()
{
}

QStringList PcPatcher::findFileToPatch() const
{
    // patch lib/pkgconfig/Qt*.pc if pkg-config is enabled, otherwise patch nothing
    // Note that pkg-config is not supported on MSVC
    if (!ArgumentsAndSettings::crossMkspec().contains(QStringLiteral("msvc"))) {
        QDir pcDir(ArgumentsAndSettings::qtDir());
        if (!pcDir.cd(QStringLiteral("lib")))
            return QStringList();
        if (!pcDir.cd(QStringLiteral("pkgconfig")))
            return QStringList();

        pcDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
        pcDir.setNameFilters({QStringLiteral("Qt*.pc"), QStringLiteral("phonon.pc")});
        if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
            pcDir.setNameFilters({QStringLiteral("Qt5*.pc"), QStringLiteral("Enginio.pc")});
        else
            pcDir.setNameFilters({QStringLiteral("Qt*.pc"), QStringLiteral("phonon.pc")});
        QStringList r;
        QStringList l = pcDir.entryList();
        foreach (const QString &f, l) {
            if ((ArgumentsAndSettings::qtQVersion().majorVersion() == 4) && f.startsWith(QStringLiteral("Qt5")))
                continue;

            if (shouldPatch(f))
                r << (QStringLiteral("lib/pkgconfig/") + f);
        }
        return r;
    }

    return QStringList();
}

bool PcPatcher::patchFile(const QString &file) const
{
    QDir qtDir(ArgumentsAndSettings::qtDir());
    QDir newDir(ArgumentsAndSettings::newDir());

    QFile f(qtDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray toWrite;
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString str = QString::fromUtf8(arr);
            str = str.trimmed();
            if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5) {
                patchQt5(str, arr, newDir);
            } else if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                // Why MinGW versions and Linux versions are different........
                if (ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("win32-")))
                    patchQt4MinGW(str, arr, newDir, QFileInfo(f).baseName());
                else
                    patchQt4Unix(str, arr, newDir, QFileInfo(f).baseName());
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

bool PcPatcher::shouldPatch(const QString &file) const
{
    QDir pcDir(ArgumentsAndSettings::qtDir());
    if (!pcDir.cd(QStringLiteral("lib")))
        return false;
    if (!pcDir.cd(QStringLiteral("pkgconfig")))
        return false;

    QDir oldDir(ArgumentsAndSettings::oldDir());

    QFile f(pcDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString str = QString::fromUtf8(arr);
            str = str.trimmed();
            if (str.startsWith(QStringLiteral("prefix="))) {
                str = str.mid(7).trimmed();
                if (QDir(QString(str).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldDir) {
                    f.close();
                    return true;
                }
            }
        }
        f.close();
    }

    return false;
}

void PcPatcher::patchQt5(const QString &_str, char *arr, const QDir &newDir) const
{
    QString str = _str;
    if (str.startsWith(QStringLiteral("prefix="))) {
        str = QStringLiteral("prefix=") + QDir::toNativeSeparators(newDir.absolutePath()).replace(QStringLiteral("\\"), QStringLiteral("\\\\")) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("libdir=")))
        strcpy(arr, "libdir=${prefix}/lib\n");
    else if (str.startsWith(QStringLiteral("includedir=")))
        strcpy(arr, "includedir=${prefix}/include\n");
}

void PcPatcher::patchQt4MinGW(const QString &_str, char *arr, const QDir &newDir, const QString &fBaseName) const
{
    QString str = _str;
    if (str.startsWith(QStringLiteral("prefix="))) {
        str = QStringLiteral("prefix=") + QDir::toNativeSeparators(newDir.absolutePath()) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("libdir="))) {
        str = QStringLiteral("libdir=") + QDir::fromNativeSeparators(newDir.absolutePath() + QStringLiteral("/lib")) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("includedir="))) {
        str = QStringLiteral("includedir=") + QDir::fromNativeSeparators(newDir.absolutePath() + QStringLiteral("/include/") + fBaseName) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("moc_location="))) {
        str = QStringLiteral("moc_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + QStringLiteral("/bin/moc")) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("uic_location="))) {
        str = QStringLiteral("uic_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + QStringLiteral("/bin/uic")) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("rcc_location="))) {
        str = QStringLiteral("rcc_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + QStringLiteral("/bin/rcc")) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("lupdate_location="))) {
        str = QStringLiteral("lupdate_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + QStringLiteral("/bin/lupdate")) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("lrelease_location="))) {
        str = QStringLiteral("lrelease_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + QStringLiteral("/bin/lrelease")) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("Cflags:"))) {
        str = str.mid(7);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        QStringList l = str.split(QStringLiteral(" "), QString::SkipEmptyParts);
#else
        QStringList l = str.split(QStringLiteral(" "), Qt::SkipEmptyParts);
#endif
        QStringList r;
        foreach (const QString &m, l) {
            QString n = m;
            if (n.startsWith(QStringLiteral("-I"))) {
                QDir newIncludeDir(ArgumentsAndSettings::newDir() + QStringLiteral("/include"));
                QDir oldIncludeDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/include"));
                if (QDir(n.mid(2)).isAbsolute() && QDir(n.mid(2)).absolutePath() == oldIncludeDir.absolutePath())
                    n = QStringLiteral("-I") + QDir::fromNativeSeparators(newIncludeDir.absolutePath());
            }
            r << n;
        }
        str = QStringLiteral("Cflags: ") + r.join(QLatin1Char(' ')) + QStringLiteral(" \n");
        strcpy(arr, str.toUtf8().constData());
    }
}

void PcPatcher::patchQt4Unix(const QString &_str, char *arr, const QDir &newDir, const QString &fBaseName) const
{
    QString str = _str;
    if (str.startsWith(QStringLiteral("prefix="))) {
        str = QStringLiteral("prefix=") + QDir::fromNativeSeparators(newDir.absolutePath()) + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("libdir=")))
        strcpy(arr, "libdir=${prefix}/lib\n");
    else if (str.startsWith(QStringLiteral("includedir="))) {
        str = QStringLiteral("includedir=${prefix}/include/") + fBaseName + QStringLiteral("\n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("moc_location=")))
        strcpy(arr, "moc_location=${prefix}/bin/moc\n");
    else if (str.startsWith(QStringLiteral("uic_location=")))
        strcpy(arr, "uic_location=${prefix}/bin/uic\n");
    else if (str.startsWith(QStringLiteral("rcc_location=")))
        strcpy(arr, "rcc_location=${prefix}/bin/rcc\n");
    else if (str.startsWith(QStringLiteral("lupdate_location=")))
        strcpy(arr, "lupdate_location=${prefix}/bin/lupdate\n");
    else if (str.startsWith(QStringLiteral("lrelease_location=")))
        strcpy(arr, "lrelease_location=${prefix}/bin/lrelease\n");
    else if (str.startsWith(QStringLiteral("Libs.private:"))) {
        str = str.mid(13);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        QStringList l = str.split(QStringLiteral(" "), QString::SkipEmptyParts);
#else
        QStringList l = str.split(QStringLiteral(" "), Qt::SkipEmptyParts);
#endif
        QStringList r;
        foreach (const QString &m, l) {
            QString n = m;
            QDir newLibDir(ArgumentsAndSettings::newDir() + QStringLiteral("/lib"));
            QDir oldLibDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/lib"));
            if (n.startsWith(QStringLiteral("-L"))) {
                if (QDir(n.mid(2).replace(QStringLiteral("\\\\"), QStringLiteral("\\"))) == oldLibDir)
                    n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
            } else if (!n.startsWith(QStringLiteral("-l"))) {
                QFileInfo fi(QString(n).replace(QStringLiteral("\\\\"), QStringLiteral("\\")));
                if (fi.isAbsolute() && QDir(fi.absolutePath()) == oldLibDir) {
                    QFileInfo fiNew(newLibDir, fi.baseName());
                    n = QDir::fromNativeSeparators(fiNew.absoluteFilePath());
                }
            }
            r << n;
        }
        str = QStringLiteral("Libs.private: ") + r.join(QLatin1Char(' ')) + QStringLiteral(" \n");
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith(QStringLiteral("Cflags:"))) {
        str = str.mid(7);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        QStringList l = str.split(QStringLiteral(" "), QString::SkipEmptyParts);
#else
        QStringList l = str.split(QStringLiteral(" "), Qt::SkipEmptyParts);
#endif
        QStringList r;
        foreach (const QString &m, l) {
            QString n = m;
            if (n.startsWith(QStringLiteral("-I"))) {
                QDir newIncludeDir(ArgumentsAndSettings::newDir() + QStringLiteral("/include"));
                QDir oldIncludeDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/include"));
                if (QDir(n.mid(2)).isAbsolute() && QDir(n.mid(2)).absolutePath() == oldIncludeDir.absolutePath())
                    n = QStringLiteral("-I") + QDir::fromNativeSeparators(newIncludeDir.absolutePath());
            }
            r << n;
        }
        str = QStringLiteral("Cflags: ") + r.join(QLatin1Char(' ')) + QStringLiteral(" \n");
        strcpy(arr, str.toUtf8().constData());
    }
}

REGISTER_PATCHER(PcPatcher)

#include "pc.moc"
