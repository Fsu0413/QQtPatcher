// SPDX-License-Identifier: Unlicense

#ifndef QQBPLOG_H
#define QQBPLOG_H

#include <QString>

struct QbpLogPrivate;

class QbpLog
{
public:
    enum LogLevel
    {
        Verbose,
        Warning,
        Error,
        Fatal
    };

    static QbpLog &instance();
    ~QbpLog();
    void setVerbose(bool verbose);
    bool setLogFile(const QString &fileName);
    void print(const QString &c, LogLevel l = Verbose);

private:
    QbpLog();
    Q_DISABLE_COPY(QbpLog)

    QbpLogPrivate *d;
};

Q_ALWAYS_INLINE void QBPLOGV(const QString &l)
{
    QbpLog::instance().print(l, QbpLog::Verbose);
}

Q_ALWAYS_INLINE void QBPLOGW(const QString &l)
{
    QbpLog::instance().print(l, QbpLog::Warning);
}

Q_ALWAYS_INLINE void QBPLOGE(const QString &l)
{
    QbpLog::instance().print(l, QbpLog::Error);
}

Q_ALWAYS_INLINE void QBPLOGF(const QString &l)
{
    QbpLog::instance().print(l, QbpLog::Fatal);
}

#endif
