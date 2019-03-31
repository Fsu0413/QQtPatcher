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

    bool isQt4File(const QString &fileName) const;
    QStringList findFileToPatch4() const;
    QStringList findFileToPatch5() const;

    bool shouldPatch(const QString &file) const;
};

PcPatcher::PcPatcher()
{
}

PcPatcher::~PcPatcher()
{
}

QStringList PcPatcher::findFileToPatch() const
{
    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
        return findFileToPatch5();
    else if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4)
        return findFileToPatch4();

    return QStringList();
}

bool PcPatcher::isQt4File(const QString &fileName) const
{
    return !QFileInfo(fileName).fileName().startsWith("Qt5");
}

QStringList PcPatcher::findFileToPatch4() const
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
        QStringList r;
        QStringList l = pcDir.entryList();
        foreach (const QString &f, l) {
            if (isQt4File(f) && shouldPatch(f))
                r << (QStringLiteral("lib/pkgconfig/") + f);
        }
        return r;
    }

    return QStringList();
}

QStringList PcPatcher::findFileToPatch5() const
{
    // patch lib/pkgconfig/Qt5*.pc if pkg-config is enabled, otherwise patch nothing
    if (!ArgumentsAndSettings::crossMkspec().startsWith("win32-msvc")) {
        QDir pcDir(ArgumentsAndSettings::qtDir());
        if (!pcDir.cd("lib"))
            return QStringList();
        if (!pcDir.cd("pkgconfig"))
            return QStringList();

        pcDir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable);
        pcDir.setNameFilters({"Qt5*.pc", "Enginio.pc"});
        QStringList r;
        QStringList l = pcDir.entryList();
        foreach (const QString &f, l) {
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
                if (str.startsWith("prefix=")) {
                    str = QStringLiteral("prefix=") + QDir::toNativeSeparators(newDir.absolutePath()).replace("\\", "\\\\") + "\n";
                    strcpy(arr, str.toUtf8().constData());
                } else if (str.startsWith("libdir="))
                    strcpy(arr, "libdir=${prefix}/lib\n");
                else if (str.startsWith("includedir="))
                    strcpy(arr, "includedir=${prefix}/include\n");
            } else if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
                if (str.startsWith("prefix=")) {
                    str = QStringLiteral("prefix=") + QDir::toNativeSeparators(newDir.absolutePath()) + "\n";
                    strcpy(arr, str.toUtf8().constData());
                } else if (str.startsWith("libdir=")) {
                    str = QStringLiteral("libdir=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/lib") + "\n";
                    strcpy(arr, str.toUtf8().constData());
                } else if (str.startsWith("includedir=")) {
                    str = QStringLiteral("includedir=") + QDir::fromNativeSeparators(newDir.absolutePath() + "/include/" + QFileInfo(f).baseName()) + "\n";
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

REGISTER_PATCHER(PcPatcher)

#include "pc.moc"
