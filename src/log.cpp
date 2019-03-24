#include "log.h"
#include <QDebug>
#include <QFile>

struct QbpLogPrivate
{
    QFile f;
    bool verbose;

    QbpLogPrivate()
        : verbose(false)
    {
    }
};

QbpLog &QbpLog::instance()
{
    static QbpLog l;
    return l;
}

QbpLog::~QbpLog()
{
    if (d->f.isOpen())
        d->f.close();

    delete d;
}

void QbpLog::setVerbose(bool verbose)
{
    d->verbose = verbose;
}

bool QbpLog::setLogFile(const QString &fileName)
{
    if (d->f.isOpen())
        d->f.close();

    if (!fileName.isEmpty()) {
        d->f.setFileName(fileName);
        return d->f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    }

    return true;
}

void QbpLog::print(const QString &c, LogLevel l)
{
    bool levelAvailable = true;
    bool fatalError = false;
    static const QStringList logLevelStr {"Verbose", "Warning", "Error", "Fatal"};
    switch (l) {
    case Verbose:
        if (d->verbose)
            qDebug(c.toUtf8().constData());
        break;
    case Warning:
        qWarning(c.toUtf8().constData());
        break;
    case Error:
        qCritical(c.toUtf8().constData());
        break;
    case Fatal:
        // qFatal will call exit();
        fatalError = true;
        break;
    default:
        // ???
        levelAvailable = false;
        print(QString("Log Level %1 is not available for log \"%2\"").arg(static_cast<int>(l)).arg(c), Warning);
        break;
    }

    if (levelAvailable && d->f.isOpen())
        d->f.write(QString("%1:%2\n").arg(logLevelStr.value(static_cast<int>(l))).arg(c).toUtf8().constData());

    if (fatalError)
        qFatal(c.toUtf8().constData());
}

QbpLog::QbpLog()
    : d(new QbpLogPrivate)
{
}
