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
    if (ArgumentsAndSettings::hostMkspec().startsWith(QStringLiteral("win32"))) {
        if (qtDir.exists(QStringLiteral("bin/qmake.exe")))
            r << QStringLiteral("bin/qmake.exe");
        else
            QBPLOGW(QStringLiteral("Cannot find bin/qmake.exe"));
    } else {
        if (qtDir.exists(QStringLiteral("bin/qmake")))
            r << QStringLiteral("bin/qmake");
        else
            QBPLOGW(QStringLiteral("Cannot find bin/qmake"));
    }

    if (ArgumentsAndSettings::hostMkspec() == ArgumentsAndSettings::crossMkspec()) {
        bool exist = false;

        // for non-cross shared/dynamic builds, search QtCore4.dll/QtCored4.dll(on Windows),
        // or libQtCore.so(on Unix-like system such as linux), libQtCore.dylib(on macOS)
        if (ArgumentsAndSettings::hostMkspec().startsWith(QStringLiteral("win32"))) {
            if (qtDir.exists(QStringLiteral("bin/QtCore4.dll"))) {
                exist = true;
                r << QStringLiteral("bin/QtCore4.dll");
            } else
                n << QStringLiteral("bin/QtCore4.dll");
            if (qtDir.exists(QStringLiteral("bin/QtCored4.dll"))) {
                exist = true;
                r << QStringLiteral("bin/QtCored4.dll");
            } else
                n << QStringLiteral("bin/QtCored4.dll");
            if (qtDir.exists(QStringLiteral("lib/QtCore4.dll"))) {
                exist = true;
                r << QStringLiteral("lib/QtCore4.dll");
            } else
                n << QStringLiteral("lib/QtCore4.dll");
            if (qtDir.exists(QStringLiteral("lib/QtCored4.dll"))) {
                exist = true;
                r << QStringLiteral("lib/QtCored4.dll");
            } else
                n << QStringLiteral("lib/QtCored4.dll");
        } else if (ArgumentsAndSettings::hostMkspec().startsWith(QStringLiteral("macx")))
            return collectBinaryFilesForQt4Mac();
        else {
            if (qtDir.exists(QStringLiteral("lib/libQtCore.so.4"))) {
                exist = true;
                r << QStringLiteral("lib/libQtCore.so.4");
            } else
                n << QStringLiteral("lib/libQtCore.so.4");
        }

        // Qt4 doesn't support static builds
        if (!exist)
            QBPLOGW(QString(QStringLiteral("Cannot find QtCore4 dynamic/shared libs, searched files are: %1")).arg(n.join(QStringLiteral(", "))));
    }

    return r;
}

QStringList BinaryPatcher::findFileToPatch5() const
{
    QStringList r;
    QStringList n;

    // qmake or qmake.exe
    QDir qtDir(ArgumentsAndSettings::qtDir());
    if (ArgumentsAndSettings::hostMkspec().startsWith(QStringLiteral("win32"))) {
        if (qtDir.exists(QStringLiteral("bin/qmake.exe")))
            r << QStringLiteral("bin/qmake.exe");
        else
            QBPLOGW(QStringLiteral("Cannot find bin/qmake.exe"));
    } else {
        if (qtDir.exists(QStringLiteral("bin/qmake")))
            r << QStringLiteral("bin/qmake");
        else
            QBPLOGW(QStringLiteral("Cannot find bin/qmake"));
    }

    if (ArgumentsAndSettings::hostMkspec() == ArgumentsAndSettings::crossMkspec()) {
        bool exist = false;

        // for non-cross shared/dynamic builds, search Qt5Core.dll/Qt5Cored.dll(on Windows),
        // or libQt5Core.so(on Unix-like system such as linux), libQt5Core.dylib(on macOS)
        if (ArgumentsAndSettings::hostMkspec().startsWith(QStringLiteral("win32"))) {
            if (qtDir.exists(QStringLiteral("bin/Qt5Core.dll"))) {
                exist = true;
                r << QStringLiteral("bin/Qt5Core.dll");
            } else
                n << QStringLiteral("bin/Qt5Core.dll");
            if (qtDir.exists(QStringLiteral("bin/Qt5Cored.dll"))) {
                exist = true;
                r << QStringLiteral("bin/Qt5Cored.dll");
            } else
                n << QStringLiteral("bin/Qt5Cored.dll");
        } else if (ArgumentsAndSettings::hostMkspec().startsWith(QStringLiteral("macx"))) {
            // treat with framework/framework-less builds
            if (qtDir.exists(QStringLiteral("lib/QtCore.framework/QtCore"))) {
                exist = true;
                r << QStringLiteral("lib/QtCore.framework/QtCore");
            } else {
                n << QStringLiteral("lib/QtCore.framework/QtCore");
                if (qtDir.exists(QStringLiteral("lib/libQt5Core.dylib"))) {
                    exist = true;
                    r << QStringLiteral("lib/libQt5Core.dylib");
                } else
                    n << QStringLiteral("lib/libQt5Core.dylib");
            }
        } else {
            // we treat other platform as linux....
            if (qtDir.exists(QStringLiteral("lib/libQt5Core.so"))) {
                exist = true;
                r << QStringLiteral("lib/libQt5Core.so");
            } else
                n << QStringLiteral("lib/libQt5Core.so");
        }

        if (!exist) {
            // check for static builds, search Qt5Core.lib/Qt5Cored.lib(if build is Windows MSVC), or libQt5Core.a(otherwise...)
            if (ArgumentsAndSettings::hostMkspec().contains(QStringLiteral("msvc"))) {
                if (qtDir.exists(QStringLiteral("lib/Qt5Core.lib"))) {
                    exist = true;
                    r << QStringLiteral("lib/Qt5Core.lib");
                } else
                    n << QStringLiteral("lib/Qt5Core.lib");
                if (qtDir.exists(QStringLiteral("lib/Qt5Cored.lib"))) {
                    exist = true;
                    r << QStringLiteral("lib/Qt5Cored.lib");
                } else
                    n << QStringLiteral("lib/Qt5Core.lib");
            } else {
                if (qtDir.exists(QStringLiteral("lib/libQt5Core.a"))) {
                    exist = true;
                    r << QStringLiteral("lib/libQt5Core.a");
                } else
                    n << QStringLiteral("lib/libQt5Core.a");
            }
        }

        if (!exist)
            QBPLOGW(QString(QStringLiteral("Cannot find Qt5Core lib, searched files are: %1")).arg(n.join(QStringLiteral(", "))));
    }

    return r;

    // original QtBinPatcher patches lrelease and qdoc executables, shall we.....??
}

QStringList BinaryPatcher::collectBinaryFilesForQt4Mac() const
{
    QStringList r;

    QDir libDir(ArgumentsAndSettings::qtDir());
    if (libDir.cd(QStringLiteral("lib"))) {
        libDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::NoSymLinks);
        libDir.setNameFilters({QStringLiteral("Qt*.framework"), QStringLiteral("phonon.framework")});
        QStringList l = libDir.entryList();
        foreach (const QString &f, l) {
            QDir frameworkDir = libDir;
            if (!frameworkDir.cd(f))
                continue;

            QFileInfo fi(frameworkDir.absolutePath());
            QString baseName = fi.baseName();
            QString libPath = QStringLiteral("Versions/4/") + baseName;

            if (frameworkDir.exists(libPath))
                r << (QStringLiteral("lib/") + f + QStringLiteral("/Versions/4/") + baseName);
        }

        libDir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Readable);
        libDir.setNameFilters({QStringLiteral("libQt*.dylib"), QStringLiteral("libphonon.dylib")});
        l = libDir.entryList();
        foreach (const QString &f, l)
            r << (QStringLiteral("lib/") + f);
    }

    QDir pluginDir(ArgumentsAndSettings::qtDir());
    if (pluginDir.cd(QStringLiteral("plugins"))) {
        pluginDir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs | QDir::NoSymLinks);
        pluginDir.setNameFilters({QStringLiteral("*.dylib")});
        QFileInfoList l = pluginDir.entryInfoList();
        foreach (const QFileInfo &f, l) {
            if (f.isDir()) {
                QDir d(f.absoluteFilePath());
                d.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks);
                d.setNameFilters({QStringLiteral("*.dylib")});
                QStringList l2 = d.entryList();
                foreach (const QString &f2, l2)
                    r << (QStringLiteral("plugins/") + f.fileName() + QStringLiteral("/") + f2);
            } else
                r << (QStringLiteral("plugins/") + f.fileName());
        }
    }

    QDir binDir(ArgumentsAndSettings::qtDir());
    if (binDir.cd(QStringLiteral("bin"))) {
        // clang-format off
        static const QStringList knownQt4Apps {
            QStringLiteral("Assistant.app"),
            QStringLiteral("Designer.app"),
            QStringLiteral("Linguist.app"),
            QStringLiteral("QMLViewer.app"),
            QStringLiteral("pixeltool.app"),
            QStringLiteral("qhelpconverter.app"),
            QStringLiteral("qttracereplay.app")
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
            QString binPath = QStringLiteral("Contents/MacOS/") + baseName;

            if (appDir.exists(binPath))
                r << (QStringLiteral("bin/") + f + QStringLiteral("/Contents/MacOS/") + baseName);
        }

        // clang-format off
        static const QStringList knownQt4Tools {
            QStringLiteral("lconvert"),
            QStringLiteral("lrelease"),
            QStringLiteral("lupdate"),
            QStringLiteral("macdeployqt"),
            QStringLiteral("moc"),
            QStringLiteral("qcollectiongenerator"),
            QStringLiteral("qdoc3"),
            QStringLiteral("qhelpgenerator"),
            QStringLiteral("qmake"),
            QStringLiteral("qmlplugindump"),
            QStringLiteral("qt3to4"),
            QStringLiteral("rcc"),
            QStringLiteral("uic"),
            QStringLiteral("uic3"),
            QStringLiteral("xmlpatterns"),
            QStringLiteral("xmlpatternsvalidator")
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
    // TODO: add judgement to waitForFinished
    // Since there will be no more builds of Qt 4 from me, I might not fix this
    QProcess otool;
    otool.start(QStringLiteral("otool"), {QStringLiteral("-L"), file});
    otool.waitForFinished();

    QString output = QString::fromUtf8(otool.readAllStandardOutput());
    QBPLOGV(QStringLiteral("otool output:"));
    QBPLOGV(output);

    QStringList outputLines = output.split(QStringLiteral("\n"), QString::SkipEmptyParts);
    outputLines.removeFirst();

    QDir libDir(ArgumentsAndSettings::oldDir() + QStringLiteral("/lib"));
    QDir newDir(ArgumentsAndSettings::newDir() + QStringLiteral("/lib"));

    if (file.contains(QStringLiteral(".dylib")) || file.contains(QStringLiteral(".framework"))) {
        // The first line of their name is "LC_ID_DYLIB", it should be changed using "install_name_tool -id"
        QString id = outputLines.first();
        QString fileName = id.split(QStringLiteral("(compatibility")).first().trimmed();
        if (fileName.startsWith(QStringLiteral("/"))) {
            QBPLOGV(QStringLiteral("Qt library detected, installing LC_ID_DYLIB"));
            QString relativeToPath;
            QString path = getPathForQt4Mac(fileName, relativeToPath);

            // check that this file is in libdir, since libs in plugin dir should not install "LC_ID_DYLIB"
            if (QDir::toNativeSeparators(path) == QDir::toNativeSeparators(libDir.absolutePath())) {
                QString newPath = newDir.absoluteFilePath(relativeToPath);
                QProcess installNameTool;
                installNameTool.start(QStringLiteral("install_name_tool"), {QStringLiteral("-id"), newPath, file});
                installNameTool.waitForFinished();
                QBPLOGV(installNameTool.program() + QStringLiteral(" ") + installNameTool.arguments().join(QStringLiteral(" ")));
            }
        } else {
            QBPLOGV(QStringLiteral("Plugin detected."));
        }
        outputLines.removeFirst();
    }

    // use "install_name_tool -change" to change remaining path
    foreach (const QString &dyld, outputLines) {
        QString fileName = dyld.split(QStringLiteral("(compatibility")).first().trimmed();
        QString relativeToPath;
        QString path = getPathForQt4Mac(fileName, relativeToPath);

        // check that used file is in libdir
        if (QDir::toNativeSeparators(path) == QDir::toNativeSeparators(libDir.absolutePath())) {
            QString newPath = newDir.absoluteFilePath(relativeToPath);
            QProcess installNameTool;
            installNameTool.start(QStringLiteral("install_name_tool"), {QStringLiteral("-change"), fileName, newPath, file});
            installNameTool.waitForFinished();
            QBPLOGV(installNameTool.program() + QStringLiteral(" ") + installNameTool.arguments().join(QStringLiteral(" ")));
        }
    }
}

bool BinaryPatcher::isQmakeOrQtCoreForQt4Mac(const QString &file) const
{
    return file.endsWith(QStringLiteral("qmake")) || file.endsWith(QStringLiteral("QtCore")) || file.endsWith(QStringLiteral("QtCore.dylib"));
}

QString BinaryPatcher::getPathForQt4Mac(const QString &fileName, QString &relativeToRet) const
{
    QString r;
    if (fileName.endsWith(QStringLiteral(".dylib"))) {
        QFileInfo fileInfo(fileName);
        relativeToRet = fileInfo.fileName();
        r = fileInfo.absolutePath();
    } else if (fileName.contains(QStringLiteral(".framework"))) {
        QString frameworkName = fileName.left(fileName.lastIndexOf(QStringLiteral(".framework"))) + QStringLiteral(".framework");
        QFileInfo fileInfo(frameworkName);
        QDir d(fileInfo.absoluteDir());
        relativeToRet = d.relativeFilePath(fileName);
        r = d.absolutePath();
    } else if (fileName.contains(QStringLiteral(".app"))) {
        QString appName = fileName.left(fileName.lastIndexOf(QStringLiteral(".app"))) + QStringLiteral(".app");
        QFileInfo fileInfo(appName);
        QDir d(fileInfo.absoluteDir());
        relativeToRet = d.relativeFilePath(fileName);
        r = d.absolutePath();
    } else {
        QFileInfo fileInfo(fileName);
        relativeToRet = fileInfo.fileName();
        r = fileInfo.absolutePath();
    }
    QBPLOGV(QString(QStringLiteral("BinaryPatcher::getPathForQt4Mac: filename = %1, relativeToRet = %2, ret = %3")).arg(fileName).arg(relativeToRet).arg(r));

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
        qMakePair<QByteArray, QString>("qt_docspath=", QStringLiteral("/doc")),
        qMakePair<QByteArray, QString>("qt_hdrspath=", QStringLiteral("/include")),
        qMakePair<QByteArray, QString>("qt_libspath=", QStringLiteral("/lib")),
        qMakePair<QByteArray, QString>("qt_binspath=", QStringLiteral("/bin")),
        qMakePair<QByteArray, QString>("qt_plugpath=", QStringLiteral("/plugins")),
        qMakePair<QByteArray, QString>("qt_impspath=", QStringLiteral("/imports")),
        qMakePair<QByteArray, QString>("qt_trnspath=", QStringLiteral("/translations")),
        qMakePair<QByteArray, QString>("qt_xmplpath=", QStringLiteral("/examples")),
        qMakePair<QByteArray, QString>("qt_demopath=", QStringLiteral("/demos"))
    };
    // clang-format on

    const QList<KeySuffixPair> *l = &l5;
    if (ArgumentsAndSettings::qtQVersion().majorVersion() == 4) {
        if (ArgumentsAndSettings::hostMkspec().startsWith(QStringLiteral("macx"))) {
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
                plusPath.append('\0');
                arr.replace(index, plusPath.length(), plusPath);
                index += plusPath.length();
            }
        }
        binFile.close();
        if (binFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            binFile.write(arr);
            binFile.close();
        } else {
            QBPLOGE(QString(QStringLiteral("file %1 is not writable during patching.")).arg(binFile.fileName()));
            return false;
        }
    } else {
        QBPLOGE(QString(QStringLiteral("file %1 is not found or not readable during patching.")).arg(binFile.fileName()));
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
