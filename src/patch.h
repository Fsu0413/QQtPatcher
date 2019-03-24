#ifndef QQBPPATCH_H
#define QQBPPATCH_H

#include <QCoreApplication>
#include <QMetaObject>
#include <QObject>

class Patcher : public QObject
{
    Q_OBJECT

public:
    Patcher();
    virtual ~Patcher() = 0;

    // make sure the following functions called after prepare();
    virtual QStringList findFileToPatch() const = 0;
    virtual bool patchFile(const QString &file) const = 0;
};

void registerPatcherMetaObject(const QMetaObject *metaObject);
void warnAboutUnsupportedQtVersion();
bool shouldForce();

void prepare();
bool patch();
void cleanup();

#define REGISTER_PATCHER(c)                              \
    namespace {                                          \
    void registerPatcher_##c()                           \
    {                                                    \
        registerPatcherMetaObject(&c::staticMetaObject); \
    }                                                    \
    }                                                    \
    Q_COREAPP_STARTUP_FUNCTION(registerPatcher_##c)

#endif
