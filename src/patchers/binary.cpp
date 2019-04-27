#include "argument.h"
#include "log.h"
#include "patch.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QStringList>

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

    QStringList collectBinaryFilesForQt4Mac() const;
    void changeBinaryPathsForQt4Mac(const QString &file) const;
    bool isQmakeOrQtCoreForQt4Mac(const QString &file) const;
    QString getPathForQt4Mac(const QString &fileName, QString &relativeToRet) const;
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
        } else if (ArgumentsAndSettings::hostMkspec().startsWith("macx"))
            return collectBinaryFilesForQt4Mac();
        else {
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

QStringList BinaryPatcher::collectBinaryFilesForQt4Mac() const
{
    QStringList r;

    QDir libDir(ArgumentsAndSettings::qtDir());
    if (libDir.cd("lib")) {
        libDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::NoSymLinks);
        libDir.setNameFilters({"Qt*.framework", "phonon.framework"});
        QStringList l = libDir.entryList();
        foreach (const QString &f, l) {
            QDir frameworkDir = libDir;
            if (!frameworkDir.cd(f))
                continue;

            QFileInfo fi(frameworkDir.absolutePath());
            QString baseName = fi.baseName();
            QString libPath = "Versions/4/" + baseName;

            if (frameworkDir.exists(libPath))
                r << (QStringLiteral("lib/") + f + "/Versions/4/" + baseName);
        }

        libDir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Readable);
        libDir.setNameFilters({"libQt*.dylib", "libphonon.dylib"});
        l = libDir.entryList();
        foreach (const QString &f, l)
            r << (QStringLiteral("lib/") + f);
    }

    QDir pluginDir(ArgumentsAndSettings::qtDir());
    if (pluginDir.cd("plugins")) {
        pluginDir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs | QDir::NoSymLinks);
        pluginDir.setNameFilters({"*.dylib"});
        QFileInfoList l = pluginDir.entryInfoList();
        foreach (const QFileInfo &f, l) {
            if (f.isDir()) {
                QDir d(f.absoluteFilePath());
                d.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks);
                d.setNameFilters({"*.dylib"});
                QStringList l2 = d.entryList();
                foreach (const QString &f2, l2)
                    r << (QStringLiteral("plugins/") + f.fileName() + "/" + f2);
            } else
                r << (QStringLiteral("plugins/") + f.fileName());
        }
    }

    QDir binDir(ArgumentsAndSettings::qtDir());
    if (binDir.cd("bin")) {
        // clang-format off
        static const QStringList knownQt4Apps {
            "Assistant.app",
            "Designer.app",
            "Linguist.app",
            "QMLViewer.app",
            "pixeltool.app",
            "qhelpconverter.app",
            "qttracereplay.app"
        };
        // clang-format on

        foreach (const QString &f, knownQt4Apps) {
            if (!binDir.exists(f))
                continue;

            QDir appDir(binDir);

            if (!appDir.cd(f))
                continue;

            QFileInfo fi(appDir.absolutePath());
            QString baseName = fi.baseName();
            QString binPath = "Contents/MacOS/" + baseName;

            if (appDir.exists(binPath))
                r << (QStringLiteral("bin/") + f + "/Contents/MacOS/" + baseName);
        }

        // clang-format off
        static const QStringList knownQt4Tools {
            "lconvert",
            "lrelease",
            "lupdate",
            "macdeployqt",
            "moc",
            "qcollectiongenerator",
            "qdoc3",
            "qhelpgenerator",
            "qmake",
            "qmlplugindump",
            "qt3to4",
            "rcc",
            "uic",
            "uic3",
            "xmlpatterns",
            "xmlpatternsvalidator"
        };
        // clang-format on

        foreach (const QString &f, knownQt4Tools) {
            if (!binDir.exists(f))
                continue;

            r << (QStringLiteral("bin/") + f);
        }
    }
    return r;
}

void BinaryPatcher::changeBinaryPathsForQt4Mac(const QString &file) const
{
    QProcess otool;
    otool.start("otool", {"-L", file});
    otool.waitForFinished();

    QString output = otool.readAllStandardOutput();
    QBPLOGV("otool output:");
    QBPLOGV(output);

    QStringList outputLines = output.split("\n", QString::SkipEmptyParts);
    outputLines.removeFirst();

    QDir libDir(ArgumentsAndSettings::oldDir() + "/lib");
    QDir newDir(ArgumentsAndSettings::newDir() + "/lib");

    if (file.contains(".dylib") || file.contains(".framework")) {
        // The first line of their name is "LC_ID_DYLIB", it should be changed using "install_name_tool -id"
        QString id = outputLines.first();
        QString fileName = id.split("(compatibility").first().trimmed();
        if (fileName.startsWith("/")) {
            QBPLOGV("Qt library detected, installing LC_ID_DYLIB");
            QString relativeToPath;
            QString path = getPathForQt4Mac(fileName, relativeToPath);

            // check that this file is in libdir, since libs in plugin dir should not install "LC_ID_DYLIB"
            if (QDir::toNativeSeparators(path) == QDir::toNativeSeparators(libDir.absolutePath())) {
                QString newPath = newDir.absoluteFilePath(relativeToPath);
                QProcess installNameTool;
                installNameTool.start("install_name_tool", {"-id", newPath, file});
                installNameTool.waitForFinished();
                QBPLOGV(installNameTool.program() + " " + installNameTool.arguments().join(" "));
            }
        } else {
            QBPLOGV("Plugin detected.");
        }
        outputLines.removeFirst();
    }

    // use "install_name_tool -change" to change remaining path
    foreach (const QString &dyld, outputLines) {
        QString fileName = dyld.split("(compatibility").first().trimmed();
        QString relativeToPath;
        QString path = getPathForQt4Mac(fileName, relativeToPath);

        // check that used file is in libdir
        if (QDir::toNativeSeparators(path) == QDir::toNativeSeparators(libDir.absolutePath())) {
            QString newPath = newDir.absoluteFilePath(relativeToPath);
            QProcess installNameTool;
            installNameTool.start("install_name_tool", {"-change", fileName, newPath, file});
            installNameTool.waitForFinished();
            QBPLOGV(installNameTool.program() + " " + installNameTool.arguments().join(" "));
        }
    }
}

bool BinaryPatcher::isQmakeOrQtCoreForQt4Mac(const QString &file) const
{
    return file.endsWith("qmake") || file.endsWith("QtCore") || file.endsWith("QtCore.dylib");
}

QString BinaryPatcher::getPathForQt4Mac(const QString &fileName, QString &relativeToRet) const
{
    QString r;
    if (fileName.endsWith(".dylib")) {
        QFileInfo fileInfo(fileName);
        relativeToRet = fileInfo.fileName();
        r = fileInfo.absolutePath();
    } else if (fileName.contains(".framework")) {
        QString frameworkName = fileName.left(fileName.lastIndexOf(".framework")) + QStringLiteral(".framework");
        QFileInfo fileInfo(frameworkName);
        QDir d(fileInfo.absoluteDir());
        relativeToRet = d.relativeFilePath(fileName);
        r = d.absolutePath();
    } else if (fileName.contains(".app")) {
        QString appName = fileName.left(fileName.lastIndexOf(".app")) + QStringLiteral(".app");
        QFileInfo fileInfo(appName);
        QDir d(fileInfo.absoluteDir());
        relativeToRet = d.relativeFilePath(fileName);
        r = d.absolutePath();
    } else {
        QFileInfo fileInfo(fileName);
        relativeToRet = fileInfo.fileName();
        r = fileInfo.absolutePath();
    }
    QBPLOGV(QString("BinaryPatcher::getPathForQt4Mac: filename = %1, relativeToRet = %2, ret = %3").arg(fileName).arg(relativeToRet).arg(r));

    return r;
}

bool BinaryPatcher::patchFile(const QString &file) const
{
    typedef QPair<QByteArray, QString> KeySuffixPair;

    // clang-format off
    static const QList<KeySuffixPair> l5 {
        qMakePair<QByteArray, QString>("qt_epfxpath=", QString()),
        qMakePair<QByteArray, QString>("qt_prfxpath=", QString()),
        qMakePair<QByteArray, QString>("qt_hpfxpath=", QString())
    };
    static const QList<KeySuffixPair> l4 {
        qMakePair<QByteArray, QString>("qt_prfxpath=", QString()),
        qMakePair<QByteArray, QString>("qt_datapath=", QString()),
        qMakePair<QByteArray, QString>("qt_docspath=", "/doc"),
        qMakePair<QByteArray, QString>("qt_hdrspath=", "/include"),
        qMakePair<QByteArray, QString>("qt_libspath=", "/lib"),
        qMakePair<QByteArray, QString>("qt_binspath=", "/bin"),
        qMakePair<QByteArray, QString>("qt_plugpath=", "/plugins"),
        qMakePair<QByteArray, QString>("qt_impspath=", "/imports"),
        qMakePair<QByteArray, QString>("qt_trnspath=", "/translations"),
        qMakePair<QByteArray, QString>("qt_xmplpath=", "/examples"),
        qMakePair<QByteArray, QString>("qt_demopath=", "/demos")
    };
    // clang-format on

    const QList<KeySuffixPair> *l = &l5;
    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
        if (ArgumentsAndSettings::hostMkspec().startsWith("macx")) {
            changeBinaryPathsForQt4Mac(file);
            if (!isQmakeOrQtCoreForQt4Mac(file))
                return true;
        }
        l = &l4;
    }

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
