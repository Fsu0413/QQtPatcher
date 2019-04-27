#include "argument.h"
#include "patch.h"
#include <QDir>

class QMakeConfPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE QMakeConfPatcher();
    ~QMakeConfPatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;
};

QMakeConfPatcher::QMakeConfPatcher()
{
}

QMakeConfPatcher::~QMakeConfPatcher()
{
}

QStringList QMakeConfPatcher::findFileToPatch() const
{
    // This is a patcher only for QTBUG-27593 in Qt4
    if (ArgumentsAndSettings::qtQVersion().majorVersion() != 4)
        return QStringList();

    QDir qtDir(ArgumentsAndSettings::qtDir());
    if (qtDir.exists(QStringLiteral("mkspecs/default/qmake.conf")))
        return {QStringLiteral("mkspecs/default/qmake.conf")};

    return QStringList();
}

bool QMakeConfPatcher::patchFile(const QString &file) const
{
    QString str = QString(QStringLiteral("QMAKESPEC_ORIGINAL=%1/mkspecs/%2\n"
                                         "\n"
                                         "include(../%2/qmake.conf)\n"))
                      .arg(ArgumentsAndSettings::newDir())
                      .arg(ArgumentsAndSettings::crossMkspec());

    QFile f(file);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    f.write(str.toUtf8());
    f.close();

    return true;
}

REGISTER_PATCHER(QMakeConfPatcher)

#include "qmakeconf.moc"
