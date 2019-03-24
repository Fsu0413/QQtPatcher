#ifndef QQBPARGUMENT_H
#define QQBPARGUMENT_H

#include <QString>

namespace ArgumentsAndSettings {

bool parse();

// parameters
bool verbose();
QString backupDir();
QString logFile();
bool force();
QString qtDir();
QString newDir();
bool dryRun();
QStringList unknownParameters();

// config files
QString crossMkspec();
QString hostMkspec();
QString qtVersion();

// detected from QMake
QString oldDir();

// helpers
void setQtDir(const QString &dir);
void setNewDir(const QString &dir);
void setCrossMkspec(const QString &mkspec);
void setHostMkspec(const QString &mkspec);
void setQtVersion(const QString &version);
void setOldDir(const QString &dir);

}

#endif
