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
};

PcPatcher::PcPatcher()
{
}

PcPatcher::~PcPatcher()
{
}

QStringList PcPatcher::findFileToPatch() const
{
    // patch lib/pkgconfig/Qt5*.pc if pkg-config is enabled, otherwise patch nothing
    // don't know whether MinGW versions supports pkg-config, need research.
    // Assuming no support for pkg-config in MinGW version for now

    if (!ArgumentsAndSettings::crossMkspec().startsWith("win")) {
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
            if (str.startsWith("prefix=")) {
                str = QStringLiteral("prefix=") + QDir::toNativeSeparators(newDir.absolutePath()).replace("\\", "\\\\") + "\n";
                strcpy(arr, str.toUtf8().constData());
            } else if (str.startsWith("libdir="))
                strcpy(arr, "libdir=${prefix}/lib\n");
            else if (str.startsWith("includedir="))
                strcpy(arr, "includedir=${prefix}/include\n");

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
                if (QDir(str.replace("\\\\", "\\")) == oldDir) {
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
