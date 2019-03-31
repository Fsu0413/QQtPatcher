#include "argument.h"
#include "log.h"
#include "patch.h"
#include <QDir>
#include <QFile>

class BinaryPatcher : public Patcher
{
    Q_OBJECT

public:
    Q_INVOKABLE BinaryPatcher();
    ~BinaryPatcher() override;

    QStringList findFileToPatch() const override;
    bool patchFile(const QString &file) const override;

    QStringList findFileToPatch4() const;
    QStringList findFileToPatch5() const;
};

BinaryPatcher::BinaryPatcher()
{
}

BinaryPatcher::~BinaryPatcher()
{
}

QStringList BinaryPatcher::findFileToPatch() const
{
    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
        return findFileToPatch5();
    else if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4)
        return findFileToPatch4();

    return QStringList();
}

QStringList BinaryPatcher::findFileToPatch4() const
{
    QStringList r;
    QStringList n;

    // qmake or qmake.exe
    QDir qtDir(ArgumentsAndSettings::qtDir());
    if (ArgumentsAndSettings::hostMkspec().startsWith("win32")) {
        if (qtDir.exists("bin/qmake.exe"))
            r << "bin/qmake.exe";
        else
            QBPLOGW("Cannot find bin/qmake.exe");
    } else {
        if (qtDir.exists("bin/qmake"))
            r << "bin/qmake";
        else
            QBPLOGW("Cannot find bin/qmake");
    }

    if (ArgumentsAndSettings::hostMkspec() == ArgumentsAndSettings::crossMkspec()) {
        bool exist = false;

        // for non-cross shared/dynamic builds, search QtCore4.dll/QtCored4.dll(on Windows),
        // or libQtCore.so(on Unix-like system such as linux), libQtCore.dylib(on macOS)
        if (ArgumentsAndSettings::hostMkspec().startsWith("win32")) {
            if (qtDir.exists("bin/QtCore4.dll")) {
                exist = true;
                r << "bin/QtCore4.dll";
            } else
                n << "bin/QtCore4.dll";
            if (qtDir.exists("bin/QtCored4.dll")) {
                exist = true;
                r << "bin/QtCored4.dll";
            } else
                n << "bin/QtCored4.dll";
            if (qtDir.exists("lib/QtCore4.dll")) {
                exist = true;
                r << "lib/QtCore4.dll";
            } else
                n << "lib/QtCore4.dll";
            if (qtDir.exists("lib/QtCored4.dll")) {
                exist = true;
                r << "lib/QtCored4.dll";
            } else
                n << "lib/QtCored4.dll";
        } else if (ArgumentsAndSettings::hostMkspec().startsWith("macx")) {
            QBPLOGW("Todo....");
        } else {
            if (qtDir.exists("lib/libQtCore.so.4")) {
                exist = true;
                r << "lib/libQtCore.so.4";
            } else
                n << "lib/libQtCore.so.4";
        }

        // Qt4 doesn't support static builds
        if (!exist)
            QBPLOGW(QString("Cannot find QtCore4 dynamic/shared libs, searched files are: %1").arg(n.join(", ")));
    }

    return r;
}

QStringList BinaryPatcher::findFileToPatch5() const
{
    QStringList r;
    QStringList n;

    // qmake or qmake.exe
    QDir qtDir(ArgumentsAndSettings::qtDir());
    if (ArgumentsAndSettings::hostMkspec().startsWith("win32")) {
        if (qtDir.exists("bin/qmake.exe"))
            r << "bin/qmake.exe";
        else
            QBPLOGW("Cannot find bin/qmake.exe");
    } else {
        if (qtDir.exists("bin/qmake"))
            r << "bin/qmake";
        else
            QBPLOGW("Cannot find bin/qmake");
    }

    if (ArgumentsAndSettings::hostMkspec() == ArgumentsAndSettings::crossMkspec()) {
        bool exist = false;

        // for non-cross shared/dynamic builds, search Qt5Core.dll/Qt5Cored.dll(on Windows),
        // or libQt5Core.so(on Unix-like system such as linux), libQt5Core.dylib(on macOS)
        if (ArgumentsAndSettings::hostMkspec().startsWith("win32")) {
            if (qtDir.exists("bin/Qt5Core.dll")) {
                exist = true;
                r << "bin/Qt5Core.dll";
            } else
                n << "bin/Qt5Core.dll";
            if (qtDir.exists("bin/Qt5Cored.dll")) {
                exist = true;
                r << "bin/Qt5Cored.dll";
            } else
                n << "bin/Qt5Cored.dll";
        } else if (ArgumentsAndSettings::hostMkspec().startsWith("macx")) {
            // treat with framework/framework-less builds
            if (qtDir.exists("lib/QtCore.framework/QtCore")) {
                exist = true;
                r << "lib/QtCore.framework/QtCore";
            } else {
                n << "lib/QtCore.framework/QtCore";
                if (qtDir.exists("lib/libQt5Core.dylib")) {
                    exist = true;
                    r << "lib/libQt5Core.dylib";
                } else
                    n << "lib/libQt5Core.dylib";
            }
        } else {
            // we treat other platform as linux....
            if (qtDir.exists("lib/libQt5Core.so")) {
                exist = true;
                r << "lib/libQt5Core.so";
            } else
                n << "lib/libQt5Core.so";
        }

        if (!exist) {
            // check for static builds, search Qt5Core.lib/Qt5Cored.lib(if build is Windows MSVC), or libQt5Core.a(otherwise...)
            if (ArgumentsAndSettings::hostMkspec().contains("msvc")) {
                if (qtDir.exists("lib/Qt5Core.lib")) {
                    exist = true;
                    r << "lib/Qt5Core.lib";
                } else
                    n << "lib/Qt5Core.lib";
                if (qtDir.exists("lib/Qt5Cored.lib")) {
                    exist = true;
                    r << "lib/Qt5Cored.lib";
                } else
                    n << "lib/Qt5Core.lib";
            } else {
                if (qtDir.exists("lib/libQt5Core.a")) {
                    exist = true;
                    r << "lib/libQt5Core.a";
                } else
                    n << "lib/libQt5Core.a";
            }
        }

        if (!exist)
            QBPLOGW(QString("Cannot find Qt5Core lib, searched files are: %1").arg(n.join(", ")));
    }

    return r;

    // original QtBinPatcher patches lrelease and qdoc executables, shall we.....??
}

bool BinaryPatcher::patchFile(const QString &file) const
{
    typedef QPair<QByteArray, QString> KeySuffixPair;

    static const QList<KeySuffixPair> l5 {qMakePair<QByteArray, QString>("qt_epfxpath=", QString()), qMakePair<QByteArray, QString>("qt_prfxpath=", QString()),
                                          qMakePair<QByteArray, QString>("qt_hpfxpath=", QString())};
    static const QList<KeySuffixPair> l4 {qMakePair<QByteArray, QString>("qt_prfxpath=", QString()),       qMakePair<QByteArray, QString>("qt_datapath=", QString()),
                                          qMakePair<QByteArray, QString>("qt_docspath=", "/doc"),          qMakePair<QByteArray, QString>("qt_hdrspath=", "/include"),
                                          qMakePair<QByteArray, QString>("qt_libspath=", "/lib"),          qMakePair<QByteArray, QString>("qt_binspath=", "/bin"),
                                          qMakePair<QByteArray, QString>("qt_plugpath=", "/plugins"),      qMakePair<QByteArray, QString>("qt_impspath=", "/imports"),
                                          qMakePair<QByteArray, QString>("qt_trnspath=", "/translations"), qMakePair<QByteArray, QString>("qt_xmplpath=", "/examples"),
                                          qMakePair<QByteArray, QString>("qt_demopath=", "/demos")};

    const QList<KeySuffixPair> *l = &l5;
    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4)
        l = &l4;

    QDir qtDir(ArgumentsAndSettings::qtDir());
    QFile binFile(qtDir.absoluteFilePath(file));
    if (binFile.exists() && binFile.open(QIODevice::ReadOnly)) {
        QByteArray arr = binFile.readAll();
        foreach (const KeySuffixPair &i, *l) {
            int index = 0;
            while ((index = arr.indexOf(i.first, index)) != -1) {
                QByteArray plusPath = i.first;
                if (ArgumentsAndSettings::qtQVersion().majorVersion() == 5)
                    plusPath.append(QDir::fromNativeSeparators(QDir(ArgumentsAndSettings::newDir() + i.second).absolutePath()).toUtf8());
                else
                    plusPath.append(QDir::toNativeSeparators(QDir(ArgumentsAndSettings::newDir() + i.second).absolutePath()).toUtf8());
                arr.replace(index, plusPath.length(), plusPath);
                index += plusPath.length();
            }
        }
        binFile.close();
        if (binFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            binFile.write(arr);
            binFile.close();
        } else {
            QBPLOGE(QString("file %1 is not writable during patching.").arg(binFile.fileName()));
            return false;
        }
    } else {
        QBPLOGE(QString("file %1 is not found or not readable during patching.").arg(binFile.fileName()));
        return false;
    }
    return true;

    // orginal QtBinPatcher patches the following variables, which I didn't found in the binary. Maybe these variables are in Qt4 Unix or Cross-compiled versions?
    // "qt_adatpath=",
    // "qt_lbexpath=",
    // "qt_tstspath=",
    // "qt_qml2path=",
    // "qt_hdatpath=",
    // "qt_hbinpath=",
    // "qt_hlibpath="
}

REGISTER_PATCHER(BinaryPatcher)

#include "binary.moc"
