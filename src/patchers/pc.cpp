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

    void patchQt5(const QString &str, char *arr, const QDir &newDir);
    void patchQt4MinGW(const QString &str, char *arr, const QDir &newDir, const QString &fBaseName);
    void patchQt4Unix(const QString &str, char *arr, const QDir &newDir, const QString &fBaseName);
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
    if (!ArgumentsAndSettings::crossMkspec().startsWith("win32-msvc")) {
        QDir pcDir(ArgumentsAndSettings::qtDir());
        if (!pcDir.cd("lib"))
            return QStringList();
        if (!pcDir.cd("pkgconfig"))
            return QStringList();

        pcDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
        pcDir.setNameFilters({"Qt*.pc", "phonon.pc"});
        if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
            pcDir.setNameFilters({"Qt5*.pc", "Enginio.pc"});
        else
            pcDir.setNameFilters({"Qt*.pc", "phonon.pc"});
        QStringList r;
        QStringList l = pcDir.entryList();
        foreach (const QString &f, l) {
            if ((ArgumentsAndSettings::qtQVersion().majorVersion() == 4) && f.startsWith("Qt5"))
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
                patchQt5(arr, str, newDir);
            } else if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                // Why MinGW versions and Linux versions are different........
                if (ArgumentsAndSettings::crossMkspec().startsWith("win32-"))
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
    if (!pcDir.cd("lib"))
        return false;
    if (!pcDir.cd("pkgconfig"))
        return false;

    QDir oldDir(ArgumentsAndSettings::oldDir());

    QFile f(pcDir.absoluteFilePath(file));
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        char arr[10000];
        while (f.readLine(arr, 9999) > 0) {
            QString str = QString::fromUtf8(arr);
            str = str.trimmed();
            if (str.startsWith("prefix=")) {
                str = str.mid(7).trimmed();
                if (QDir(QString(str).replace("\\\\", "\\")) == oldDir) {
                    f.close();
                    return true;
                }
            }
        }
        f.close();
    }

    return false;
}

void PcPatcher::patchQt5(const QString &_str, char *arr, const QDir &newDir)
{
    QString str = _str;
    if (str.startsWith("prefix=")) {
        str = QStringLiteral("prefix=") + QDir::toNativeSeparators(newDir.absolutePath()).replace("\\", "\\\\") + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("libdir="))
        strcpy(arr, "libdir=${prefix}/lib\n");
    else if (str.startsWith("includedir="))
        strcpy(arr, "includedir=${prefix}/include\n");
}

void PcPatcher::patchQt4MinGW(const QString &_str, char *arr, const QDir &newDir, const QString &fBaseName)
{
    QString str = _str;
    if (str.startsWith("prefix=")) {
        str = QStringLiteral("prefix=") + QDir::toNativeSeparators(newDir.absolutePath()) + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("libdir=")) {
        str = QStringLiteral("libdir=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/lib") + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("includedir=")) {
        str = QStringLiteral("includedir=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/include/" + fBaseName) + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("moc_location=")) {
        str = QStringLiteral("moc_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/bin/moc") + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("uic_location=")) {
        str = QStringLiteral("uic_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/bin/uic") + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("rcc_location=")) {
        str = QStringLiteral("rcc_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/bin/rcc") + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("lupdate_location=")) {
        str = QStringLiteral("lupdate_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/bin/lupdate") + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("lrelease_location=")) {
        str = QStringLiteral("lrelease_location=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/bin/lrelease") + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("Cflags:")) {
        str = str.mid(7);
        QStringList l = str.split(" ", QString::SkipEmptyParts);
        QStringList r;
        foreach (const QString &m, l) {
            QString n = m;
            if (n.startsWith("-I")) {
                QDir newIncludeDir(ArgumentsAndSettings::newDir() + "/include");
                QDir oldIncludeDir(ArgumentsAndSettings::oldDir() + "/include");
                if (QDir(n.mid(2)).isAbsolute() && QDir(n.mid(2)).absolutePath() == oldIncludeDir.absolutePath())
                    n = QStringLiteral("-I") + QDir::fromNativeSeparators(newIncludeDir.absolutePath());
            }
            r << n;
        }
        str = QStringLiteral("Cflags: ") + r.join(' ') + " \n";
        strcpy(arr, str.toUtf8().constData());
    }
}

void PcPatcher::patchQt4Unix(const QString &_str, char *arr, const QDir &newDir, const QString &fBaseName)
{
    QString str = _str;
    if (str.startsWith("prefix=")) {
        str = QStringLiteral("prefix=") + QDir::fromNativeSeparators(newDir.absolutePath()) + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("libdir="))
        strcpy(arr, "libdir=${prefix}/lib\n");
    else if (str.startsWith("includedir=")) {
        str = QStringLiteral("includedir=${prefix}/include/") + fBaseName + "\n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("moc_location="))
        strcpy(arr, "moc_location=${prefix}/bin/moc\n");
    else if (str.startsWith("uic_location="))
        strcpy(arr, "uic_location=${prefix}/bin/uic\n");
    else if (str.startsWith("rcc_location="))
        strcpy(arr, "rcc_location=${prefix}/bin/rcc\n");
    else if (str.startsWith("lupdate_location="))
        strcpy(arr, "lupdate_location=${prefix}/bin/lupdate\n");
    else if (str.startsWith("lrelease_location="))
        strcpy(arr, "lrelease_location=${prefix}/bin/lrelease\n");
    else if (str.startsWith("Libs.private:")) {
        str = str.mid(13);
        QStringList l = str.split(" ", QString::SkipEmptyParts);
        QStringList r;
        foreach (const QString &m, l) {
            QString n = m;
            if (n.startsWith("-L")) {
                if (QDir(n.mid(2).replace("\\\\", "\\")) == oldLibDir)
                    n = QStringLiteral("-L") + QDir::fromNativeSeparators(newLibDir.absolutePath());
            } else if (!n.startsWith("-l")) {
                QFileInfo fi(QString(n).replace("\\\\", "\\"));
                if (fi.isAbsolute() && QDir(fi.absolutePath()) == oldLibDir) {
                    QFileInfo fiNew(newLibDir, fi.baseName());
                    n = QDir::fromNativeSeparators(fiNew.absoluteFilePath());
                }
            }
            r << n;
        }
        str = QStringLiteral("Libs.private: ") + r.join(' ') + " \n";
        strcpy(arr, str.toUtf8().constData());
    } else if (str.startsWith("Cflags:")) {
        str = str.mid(7);
        QStringList l = str.split(" ", QString::SkipEmptyParts);
        QStringList r;
        foreach (const QString &m, l) {
            QString n = m;
            if (n.startsWith("-I")) {
                QDir newIncludeDir(ArgumentsAndSettings::newDir() + "/include");
                QDir oldIncludeDir(ArgumentsAndSettings::oldDir() + "/include");
                if (QDir(n.mid(2)).isAbsolute() && QDir(n.mid(2)).absolutePath() == oldIncludeDir.absolutePath())
                    n = QStringLiteral("-I") + QDir::fromNativeSeparators(newIncludeDir.absolutePath());
            }
            r << n;
        }
        str = QStringLiteral("Cflags: ") + r.join(' ') + " \n";
        strcpy(arr, str.toUtf8().constData());
    }
}

REGISTER_PATCHER(PcPatcher)

#include "pc.moc"
