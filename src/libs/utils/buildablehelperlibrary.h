#ifndef BUILDABLEHELPERLIBRARY_H
#define BUILDABLEHELPERLIBRARY_H

#include "utils_global.h"

#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace Utils {

class Environment;

class QTCREATOR_UTILS_EXPORT BuildableHelperLibrary
{
public:
    // returns the full path to the first qmake, qmake-qt4, qmake4 that has
    // at least version 2.0.0 and thus is a qt4 qmake
    static QString findSystemQt(const Utils::Environment &env);
    // return true if the qmake at qmakePath is qt4 (used by QtVersion)
    static QString qtVersionForQMake(const QString &qmakePath);
    static bool checkMinimumQtVersion(const QString &qtversionString, int majorVersion, int minorVersion, int patchVersion);
    // returns something like qmake4, qmake, qmake-qt4 or whatever distributions have chosen (used by QtVersion)
    static QStringList possibleQMakeCommands();

    static QString qtInstallHeadersDir(const QString &qmakePath);
    static QString qtInstallDataDir(const QString &qmakePath);

    static QString byInstallDataHelper(const QString &mainFilename,
                                       const QStringList &installDirectories,
                                       const QStringList &validBinaryFilenames);

    static bool copyFiles(const QString &sourcePath, const QStringList &files,
                          const QString &targetDirectory, QString *errorMessage);

    static bool buildHelper(const QString &helperName, const QString &proFilename,
                            const QString &directory, const QString &makeCommand,
                            const QString &qmakeCommand, const QString &mkspec,
                            const Utils::Environment &env, const QString &targetMode,
                            QString *output, QString *errorMessage);

    static bool getHelperFileInfoFor(const QStringList &validBinaryFilenames,
                                     const QString &directory, QFileInfo* info);
};

}

#endif // BUILDABLEHELPERLIBRARY_H
