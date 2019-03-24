#include "argument.h"
#include "log.h"
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

struct AasStorage
{
    bool parsed;

    // parameters
    bool verbose;
    QString backupDir;
    QString logfile;
    bool force;
    QString qtDir;
    QString newDir;
    bool dryRun;
    QStringList unknownParameters;

    // config files
    QString crossMkspec;
    QString hostMkspec;
    QString qtVersion;

    // detected from QMake
    QString oldDir;

    AasStorage()
        : parsed(false)
        , verbose(false)
        , force(false)
        , dryRun(false)
    {
    }
};

namespace {
AasStorage s;
}

bool ArgumentsAndSettings::parse()
{
    if (s.parsed)
        return false;

    s.parsed = true;

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription("Tool for patching paths in Qt binaries.\n"
                                     "This is a reworked version, using a static compiled Qt " QT_VERSION_STR ".\n"
                                     "Frank Su, 2019. http://mogara.org\n\n"
                                     "Thanks for Yuri V. Krugloff at http://www.tver-soft.org.");

    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption({"V", "verbose"}, "Print extended runtime information."));
    parser.addOption(QCommandLineOption({"b", "backup"},
                                        "If specified, the backup files during patching will be saved to the specified "
                                        "path. If not, the backup files during patching will be deleted if succeeded.\n"
                                        "Note: the backup files will be restored if an error occurs.",
                                        "path"));
    parser.addOption(QCommandLineOption({"l", "logfile"}, "Duplicate messages into logfile with name \"name\".", "name"));
    parser.addOption(QCommandLineOption({"f", "force"}, "Force patching (without old path actuality checking)."));
    parser.addOption(QCommandLineOption({"q", "qt-dir"},
                                        "Directory, where Qt or qmake is now located (may be relative).\n"
                                        "If not specified, will be used current directory. Patcher will "
                                        "search qmake first in directory \"path\", and then in its subdir "
                                        "\"bin\". Patcher is NEVER looking qmake in other directories.\n"
                                        "WARNING: If nonstandard directory for binary files is used "
                                        "(not \"bin\"), select directory where located qmake.",
                                        "path"));
    parser.addOption(QCommandLineOption({"n", "new-dir"},
                                        "Directory where Qt will be located (may be relative).\n"
                                        "If not specified, will be used the current location.",
                                        "path"));
    parser.addOption(QCommandLineOption({"d", "dry-run"}, "Output the procedure only, do not really process the jobs."));

    parser.process(*qApp);
    if (parser.isSet("V"))
        s.verbose = true;
    if (parser.isSet("b"))
        s.backupDir = parser.value("b");
    if (parser.isSet("l"))
        s.logfile = parser.value("l");
    if (parser.isSet("f"))
        s.force = true;
    if (parser.isSet("q"))
        s.qtDir = parser.value("q");
    if (parser.isSet("n"))
        s.newDir = parser.value("n");
    if (parser.isSet("d"))
        s.dryRun = true;

    s.unknownParameters = parser.unknownOptionNames() + parser.positionalArguments();

    QFile configFile("qbp.json");
    if (configFile.exists() && configFile.open(QFile::ReadOnly | QFile::Text)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &err);
        if (err.error == QJsonParseError::NoError) {
            if (doc.isObject()) {
                QJsonObject ob = doc.object();
                if (ob.contains("crossMkspec"))
                    s.crossMkspec = ob.value("crossMkspec").toString();
                if (ob.contains("hostMkspec"))
                    s.hostMkspec = ob.value("hostMkspec").toString();
                if (ob.contains("qtVersion"))
                    s.qtVersion = ob.value("qtVersion").toString();
            }
        }
    }

    return true;
}

bool ArgumentsAndSettings::verbose()
{
    return s.verbose;
}

QString ArgumentsAndSettings::backupDir()
{
    return s.backupDir;
}

QString ArgumentsAndSettings::logFile()
{
    return s.logfile;
}

bool ArgumentsAndSettings::force()
{
    return s.force;
}

QString ArgumentsAndSettings::qtDir()
{
    return s.qtDir;
}

QString ArgumentsAndSettings::newDir()
{
    return s.newDir;
}

bool ArgumentsAndSettings::dryRun()
{
    return s.dryRun;
}

QStringList ArgumentsAndSettings::unknownParameters()
{
    return s.unknownParameters;
}

QString ArgumentsAndSettings::crossMkspec()
{
    return s.crossMkspec;
}

QString ArgumentsAndSettings::hostMkspec()
{
    return s.hostMkspec;
}

QString ArgumentsAndSettings::qtVersion()
{
    return s.qtVersion;
}

QString ArgumentsAndSettings::oldDir()
{
    return s.oldDir;
}

void ArgumentsAndSettings::setQtDir(const QString &dir)
{
    QBPLOGV("qtDir is set to " + dir);
    s.qtDir = dir;
}

void ArgumentsAndSettings::setNewDir(const QString &dir)
{
    QBPLOGV("newDir is set to " + dir);
    s.newDir = dir;
}

void ArgumentsAndSettings::setCrossMkspec(const QString &mkspec)
{
    QBPLOGV("crossMkspec is set to " + mkspec);
    s.crossMkspec = mkspec;
}

void ArgumentsAndSettings::setHostMkspec(const QString &mkspec)
{
    QBPLOGV("hostMkspec is set to " + mkspec);
    s.hostMkspec = mkspec;
}

void ArgumentsAndSettings::setQtVersion(const QString &version)
{
    QBPLOGV("qtVersion is set to " + version);
    s.qtVersion = version;
}

void ArgumentsAndSettings::setOldDir(const QString &dir)
{
    QBPLOGV("oldDir is set to " + dir);
    s.oldDir = dir;
}
