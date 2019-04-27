#include "argument.h"
#include "log.h"
#include "patch.h"
#include <QCoreApplication>
#include <QDir>

#include <cstdio>

// Our program is supposed to be compatible with at least host builds/cross builds for Android of Qt5 after 5.6 and host builds of Qt4.8

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationVersion(QStringLiteral(VERSION));
    QDir::setCurrent(a.applicationDirPath());

    ArgumentsAndSettings::parse();

    QbpLog::instance().setVerbose(ArgumentsAndSettings::verbose());
    QbpLog::instance().setLogFile(ArgumentsAndSettings::logFile());

    if (!ArgumentsAndSettings::unknownParameters().isEmpty())
        QBPLOGW(QString(QStringLiteral("Unknown Parameters: %1")).arg(ArgumentsAndSettings::unknownParameters().join(QStringLiteral(", "))));

    bool success = true;
    prepare();
    warnAboutUnsupportedQtVersion();

    if (exitWhenSpacesExist()) {
        if (!shouldForce() || ArgumentsAndSettings::force())
            success = patch();
    } else
        success = false;

    cleanup();

    return success ? 0 : 1;
}
