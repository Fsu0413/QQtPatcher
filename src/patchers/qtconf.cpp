#include "argument.h"
#include "patch.h"
#include <QDir>

class QtConfPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE QtConfPatcher();
    ~QtConfPatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;
};

QtConfPatcher::QtConfPatcher()
{
}

QtConfPatcher::~QtConfPatcher()
{
}

QStringList QtConfPatcher::findFileToPatch() const
{
    // remove bin/qt.conf if exists.

    QDir qtDir(ArgumentsAndSettings::qtDir());
    if (qtDir.exists("bin/qt.conf"))
        return {"bin/qt.conf"};

    return QStringList();
}

bool QtConfPatcher::patchFile(const QString &file) const
{
    QDir qtDir(ArgumentsAndSettings::qtDir());
    if (qtDir.exists(file)) {
        qtDir.remove(file);
        return true;
    }

    return false;
}

REGISTER_PATCHER(QtConfPatcher)

#include "qtconf.moc"
