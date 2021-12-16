// SPDX-License-Identifier: Unlicense

#include "backup.h"
#include "argument.h"
#include "log.h"
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>

struct BackupPrivate
{
    QDir qtDir;
    QDir backupDir;
    bool tempBackup;
    QStringList filesMadeBackup;

    BackupPrivate()
        : tempBackup(false)
    {
    }
};

Backup::Backup()
    : d(new BackupPrivate)
{
    if (ArgumentsAndSettings::dryRun())
        return;

    if (ArgumentsAndSettings::backupDir().isEmpty()) {
        QTemporaryDir dir;
        if (dir.isValid()) {
            dir.setAutoRemove(false);
            d->backupDir = QDir(dir.path());
            d->tempBackup = true;
        }
    } else
        d->backupDir = QDir(ArgumentsAndSettings::backupDir());

    d->qtDir = QDir(ArgumentsAndSettings::qtDir());

    QBPLOGV(QString(QStringLiteral("BackupDir: %1, QtDir: %2")).arg(d->backupDir.absolutePath()).arg(d->qtDir.absolutePath()));
}

Backup::~Backup()
{
    if (!ArgumentsAndSettings::dryRun()) {
        if (d->tempBackup)
            destroy();
    }

    delete d;
}

bool Backup::backupOneFile(const QString &pathRelativeToQtDir_)
{
    if (ArgumentsAndSettings::dryRun())
        return false;

    QString pathRelativeToQtDir = QDir::cleanPath(pathRelativeToQtDir_);
    // detect possibly erroneous backup operation?
    if (pathRelativeToQtDir.contains(QStringLiteral(".."))) {
        QBPLOGE(QString(QStringLiteral("backupOneFile: path %1 contains \"..\", will not backup.")).arg(pathRelativeToQtDir_));
        return false;
    }

    QBPLOGV(QString(QStringLiteral("making backup for file %1")).arg(pathRelativeToQtDir));

    QFileInfo fileToBackup(d->qtDir, pathRelativeToQtDir);
    QString relativeDir = d->qtDir.relativeFilePath(fileToBackup.absolutePath());
    d->backupDir.mkpath(relativeDir);
    QFile::copy(d->qtDir.absoluteFilePath(pathRelativeToQtDir), d->backupDir.absoluteFilePath(pathRelativeToQtDir));

    d->filesMadeBackup << pathRelativeToQtDir;
    return true;
}

bool Backup::restoreAll()
{
    if (ArgumentsAndSettings::dryRun())
        return false;

    foreach (const QString &file, d->filesMadeBackup) {
        QBPLOGV(QString(QStringLiteral("restoring backup file %1")).arg(file));
        QFile::copy(d->backupDir.absoluteFilePath(file), d->qtDir.absoluteFilePath(file));
    }
    d->filesMadeBackup.clear();

    return true;
}

void Backup::destroy()
{
    if (ArgumentsAndSettings::dryRun())
        return;

    QBPLOGV(QString(QStringLiteral("deleting backup dir %1")).arg(d->backupDir.absolutePath()));
    d->backupDir.removeRecursively();
    d->filesMadeBackup.clear();
}
