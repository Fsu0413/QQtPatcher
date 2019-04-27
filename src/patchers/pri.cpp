#include "argument.h"
#include "log.h"
#include "patch.h"
#include <QDir>

class PriPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE PriPatcher();
    ~PriPatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;

    void openSSLDirWarning(const QString &file) const;
    bool shouldPatch(const QString &file) const;
};

PriPatcher::PriPatcher()
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
    if (QDir(ArgumentsAndSettings::qtDir()).exists(QStringLiteral("mkspecs/modules/qt_lib_network_private.pri")))
        openSSLDirWarning(QStringLiteral("mkspecs/modules/qt_lib_network_private.pri"));

    // patch "mkspecs/modules/qt_lib_gui_private.pri" in cross versions? or only for android?
    QString fileName = QStringLiteral("mkspecs/modules/qt_lib_gui_private.pri");
    if (ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("android"))) {
        if (QDir(ArgumentsAndSettings::qtDir()).exists(fileName) && shouldPatch(fileName))
            return {fileName};
    }

    return QStringList();

    // original QtBinPatcher patches all .pri files, but I don't know why
}

bool PriPatcher::patchFile(const QString &file) const
{
    if (file.contains(QStringLiteral("qt_lib_gui"))) {
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

void PriPatcher::openSSLDirWarning(const QString &file) const
{
    if (file.contains(QStringLiteral("qt_lib_network"))) {
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
                        return;
                    }
                }
            }
            f.close();
        }
    }
}

bool PriPatcher::shouldPatch(const QString &file) const
{
    if (file.contains(QStringLiteral("qt_lib_gui"))) {
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
    }

    return false;
}

REGISTER_PATCHER(PriPatcher)

#include "pri.moc"
