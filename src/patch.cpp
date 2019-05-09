#include "patch.h"
#include "argument.h"
#include "backup.h"
#include "log.h"
#include <QDir>
#include <QList>
#include <QProcess>
#include <QStringList>
#include <QTextStream>
#include <QVersionNumber>

namespace PatcherFactory {
QList<const QMetaObject *> metaObjects;
}

Patcher::Patcher()
{
}

Patcher::~Patcher()
{
}

namespace {

QMap<Patcher *, QStringList> patcherFileMap;

// step 1: get Qt version from QMake and command line arguments, make absolute path of both dirs passed from command line
QString step1()
{
    // make absolute path
    QDir newDir;
    if (!ArgumentsAndSettings::newDir().isEmpty())
        newDir = QDir(ArgumentsAndSettings::newDir());
    newDir.makeAbsolute();

    QDir qtDir;
    if (!ArgumentsAndSettings::qtDir().isEmpty()) {
        qtDir = QDir(ArgumentsAndSettings::qtDir());
        if (!qtDir.exists())
            QBPLOGF(QString(QStringLiteral("Cannot find %1.")).arg(ArgumentsAndSettings::qtDir()));
    }
    qtDir.makeAbsolute();

    // find QMake in QtDir
    static const QStringList binQmakePaths {QStringLiteral("bin/qmake"), QStringLiteral("bin/qmake.exe")};
    static const QStringList qmakePaths {QStringLiteral("qmake"), QStringLiteral("qmake.exe")};

    bool qtDirFlag = false;
    QString qmakeProgram = QStringLiteral("qmake");
    foreach (const QString &p, binQmakePaths) {
        if (qtDir.exists(p)) {
            qmakeProgram = p;
            qtDirFlag = true;
        }
    }

    if (!qtDirFlag && qtDir.dirName() == QStringLiteral("bin")) {
        foreach (const QString &p, qmakePaths) {
            if (qtDir.exists(p)) {
                qmakeProgram = QStringLiteral("bin/") + p;
                if (qtDir.cdUp())
                    qtDirFlag = true;
            }
        }
    }

    if (!qtDirFlag)
        QBPLOGF(QString(QStringLiteral("Cannot find qmake in %1.")).arg(qtDir.absolutePath()));

    ArgumentsAndSettings::setQtDir(qtDir.absolutePath());
    ArgumentsAndSettings::setNewDir(newDir.absolutePath());

    QBPLOGV(QString(QStringLiteral("Step1:"
                                   "Found qmake Program: %1,"
                                   "Found QtDir: %2,"
                                   "New Dir: %3"))
                .arg(qmakeProgram)
                .arg(ArgumentsAndSettings::qtDir())
                .arg(ArgumentsAndSettings::newDir()));

    if (ArgumentsAndSettings::newDir().contains(QRegExp(QStringLiteral("\\s"))))
        QBPLOGF(QString(QStringLiteral("NewDir with spaces ae not supported. (%1)")).arg(ArgumentsAndSettings::newDir()));

    return qmakeProgram;
}

// step 2: Query QMake
void step2(const QString &qmakeProgram)
{
    QDir qtDir(ArgumentsAndSettings::qtDir());

    // temporily rename qt.conf for ease processing
    bool qtConfExists = qtDir.exists(QStringLiteral("bin/qt.conf"));
    if (qtConfExists) {
        qtDir.remove(QStringLiteral("bin/QQBP_qt.conf_QQBP"));
        qtDir.rename(QStringLiteral("bin/qt.conf"), QStringLiteral("bin/QQBP_qt.conf_QQBP"));
    }

    QProcess process;
    process.setProgram(qtDir.absoluteFilePath(qmakeProgram));
    process.setWorkingDirectory(qtDir.absoluteFilePath(QStringLiteral("bin")));
    process.setArguments({QStringLiteral("-query")});
    process.setReadChannelMode(QProcess::ForwardedErrorChannel);
    process.setReadChannel(QProcess::StandardOutput);
    process.start(QIODevice::ReadOnly | QIODevice::Text);
    if (!process.waitForFinished()) {
        process.kill();
        QBPLOGF(QString(QStringLiteral("%1 did not finish for 30 seconds.")).arg(qtDir.absoluteFilePath(qmakeProgram)));
    }
    if (process.exitStatus() != QProcess::NormalExit)
        QBPLOGF(QString(QStringLiteral("%1 crashed.")).arg(qtDir.absoluteFilePath(qmakeProgram)));
    if (process.exitCode() != 0)
        QBPLOGF(QString(QStringLiteral("%1 failed, exitcode = %2.")).arg(qtDir.absoluteFilePath(qmakeProgram)).arg(process.exitCode()));

    QString s = QString::fromLocal8Bit(process.readAllStandardOutput());
    QBPLOGV(QStringLiteral("QMake output:\n") + s);

    QTextStream ts(&s, QIODevice::ReadOnly | QIODevice::Text);
    while (!ts.atEnd()) {
        QString line = ts.readLine();
        int col = line.indexOf(QLatin1Char(':'));
        if (col == -1)
            continue;
        QString key = line.left(col);
        QString value = line.mid(col + 1);
        QBPLOGV(QString(QStringLiteral("key:%1,value:%2")).arg(key).arg(value));
        if (key == QStringLiteral("QMAKE_SPEC")) {
            if (!ArgumentsAndSettings::hostMkspec().isEmpty() && !value.contains(ArgumentsAndSettings::hostMkspec()))
                QBPLOGW(QString(QStringLiteral("Host Mkspec detected from QMake is %1, which may not compatible with the one written in config file(%2)."))
                            .arg(value)
                            .arg(ArgumentsAndSettings::hostMkspec()));
            ArgumentsAndSettings::setHostMkspec(value);
        } else if (key == QStringLiteral("QMAKE_XSPEC")) {
            if (!ArgumentsAndSettings::crossMkspec().isEmpty() && !value.contains(ArgumentsAndSettings::crossMkspec()))
                QBPLOGW(QString(QStringLiteral("Cross Mkspec detected from QMake is %1, which may not compatible with the one written in config file(%2)."))
                            .arg(value)
                            .arg(ArgumentsAndSettings::crossMkspec()));
            ArgumentsAndSettings::setCrossMkspec(value);
        } else if (key == QStringLiteral("QT_VERSION")) {
            if (!ArgumentsAndSettings::qtVersion().isEmpty() && (value != ArgumentsAndSettings::qtVersion()))
                QBPLOGW(QString(QStringLiteral("Qt version detected from QMake is %1, which is different with the one written in config file(%2)."))
                            .arg(value)
                            .arg(ArgumentsAndSettings::qtVersion()));
            ArgumentsAndSettings::setQtVersion(value);
        } else if (key == QStringLiteral("QT_INSTALL_PREFIX"))
            ArgumentsAndSettings::setOldDir(QDir(value).absolutePath());
    }

    if (qtConfExists)
        qtDir.rename(QStringLiteral("bin/QQBP_qt.conf_QQBP"), QStringLiteral("bin/qt.conf"));

    QBPLOGV(QString(QStringLiteral("Step2:"
                                   "hostMkspec: %1,"
                                   "crossMkspec: %2,"
                                   "qtVersion: %3,"
                                   "oldDir: %4"))
                .arg(ArgumentsAndSettings::hostMkspec())
                .arg(ArgumentsAndSettings::crossMkspec())
                .arg(ArgumentsAndSettings::qtVersion())
                .arg(ArgumentsAndSettings::oldDir()));

    if (ArgumentsAndSettings::oldDir().contains(QRegExp(QStringLiteral("\\s"))))
        QBPLOGF(QString(QStringLiteral("OldDir with spaces ae not supported. (%1)")).arg(ArgumentsAndSettings::oldDir()));
}

// step3: generate patchers
void step3()
{
    foreach (const QMetaObject *mo, PatcherFactory::metaObjects) {
        Patcher *patcher = qobject_cast<Patcher *>(mo->newInstance());
        if (patcher == nullptr)
            continue;

        QStringList l = patcher->findFileToPatch();

        QBPLOGV(QString(QStringLiteral("Step3:File found by Patcher %1:\n%2")).arg(QString::fromUtf8(patcher->metaObject()->className())).arg(l.join(QStringLiteral("\n"))));

        if (!l.isEmpty())
            patcherFileMap[patcher] = l;
        else
            delete patcher;
    }
}

// step4: patch! (with backup)
bool step4()
{
    Backup backup;
    bool fail = false;
    foreach (Patcher *patcher, patcherFileMap.keys()) {
        QStringList l = patcherFileMap.value(patcher);
        foreach (const QString &file, l) {
            backup.backupOneFile(file);
            fail = !patcher->patchFile(file);
            QbpLog::instance().print(QString(QStringLiteral("Step4:patched %1 using Patcher %2, result: %3"))
                                         .arg(file)
                                         .arg(QString::fromUtf8(patcher->metaObject()->className()))
                                         .arg(fail ? QStringLiteral("failed") : QStringLiteral("success")),
                                     fail ? QbpLog::Error : QbpLog::Verbose);

            if (fail)
                break;
        }

        if (fail)
            break;
    }

    if (fail)
        backup.restoreAll();

    return !fail;
}

}

void registerPatcherMetaObject(const QMetaObject *metaObject)
{
    PatcherFactory::metaObjects << metaObject;
}

void prepare()
{
    QString qmakeProgram = step1();
    step2(qmakeProgram);
    step3();
}

bool shouldForce()
{
    return QDir(ArgumentsAndSettings::oldDir()) == QDir(ArgumentsAndSettings::newDir());
}

void warnAboutUnsupportedQtVersion()
{
    static const QString supportHint(QStringLiteral("You are going to patch Qt%1, which is %2. Patching may silently fail.\nOur program is supposed to be compatible with at least "
                                                    "host builds/cross builds for Android of Qt5 after 5.6 and host builds of Qt4.8"));

#define SUPPORTHINT QString(supportHint)

    QVersionNumber n = ArgumentsAndSettings::qtQVersion();

    if (n.majorVersion() >= 6) {
        QBPLOGW(SUPPORTHINT.arg(ArgumentsAndSettings::qtVersion()).arg(QStringLiteral("not released by the time of writing")));
    } else if (n.majorVersion() == 5) {
        if (n.minorVersion() < 6) {
            QBPLOGW(SUPPORTHINT.arg(ArgumentsAndSettings::qtVersion()).arg(QStringLiteral("not and won't be supported")));
        } else if ((ArgumentsAndSettings::crossMkspec() != ArgumentsAndSettings::hostMkspec())) {
            // clang-format off
            static const QStringList supportedCrossMkspec {
                QStringLiteral("android-"),
                QStringLiteral("wasm-")
            };
            // clang-format on
            bool flag = false;
            foreach (const QString &sup, supportedCrossMkspec) {
                if (ArgumentsAndSettings::crossMkspec().startsWith(sup)) {
                    flag = true;
                    break;
                }
            }
            if (!flag) {
                QString version = QString(QStringLiteral("%1(with -xplatform %2")).arg(ArgumentsAndSettings::qtVersion()).arg(ArgumentsAndSettings::crossMkspec());
                QBPLOGW(SUPPORTHINT.arg(version).arg(QStringLiteral("TODO by now")));
            } else {
                // supported
            }
        } else {
            // supported
        }
    } else if (n.majorVersion() == 4) {
        if (n.minorVersion() != 8) {
            QBPLOGW(SUPPORTHINT.arg(ArgumentsAndSettings::qtVersion()).arg(QStringLiteral("not and won't be supported")));
        } else if (ArgumentsAndSettings::crossMkspec() != ArgumentsAndSettings::hostMkspec()) {
            QString version = QString(QStringLiteral("%1 cross builds")).arg(ArgumentsAndSettings::qtVersion());
            QBPLOGW(SUPPORTHINT.arg(version).arg(QStringLiteral("not and won't be supported")));
        } else if (!ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("win32-")) && !ArgumentsAndSettings::crossMkspec().startsWith(QStringLiteral("macx-"))) {
            QBPLOGW(SUPPORTHINT.arg(ArgumentsAndSettings::qtVersion()).arg(QStringLiteral("TODO by now")));
        } else {
            // supported
        }
    } else if (n.majorVersion() < 4) {
        QBPLOGW(SUPPORTHINT.arg(ArgumentsAndSettings::qtVersion()).arg(QStringLiteral("not and won't be supported")));
    } else {
        // impossible?
    }
}

bool exitWhenSpacesExist()
{
    if (ArgumentsAndSettings::oldDir().contains(QRegExp(QStringLiteral("\\s")))) {
        QBPLOGE(QStringLiteral("Old Path of Qt contains space: ") + ArgumentsAndSettings::oldDir());
        return false;
    }
    if (ArgumentsAndSettings::newDir().contains(QRegExp(QStringLiteral("\\s")))) {
        QBPLOGE(QStringLiteral("New Path of Qt contains space: ") + ArgumentsAndSettings::newDir());
        return false;
    }

    return true;
}

bool patch()
{
    return step4();
}

void cleanup()
{
    qDeleteAll(patcherFileMap.keys());
    patcherFileMap.clear();
}
