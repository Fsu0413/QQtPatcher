#ifndef QQBPBACKUP_H
#define QQBPBACKUP_H

#include <QString>

struct BackupPrivate;

class Backup
{
public:
    Backup();
    ~Backup();

    bool backupOneFile(const QString &pathRelativeToQtDir);
    bool restoreAll();
    void destroy();

private:
    Q_DISABLE_COPY(Backup)
    BackupPrivate *d;
};

#endif
