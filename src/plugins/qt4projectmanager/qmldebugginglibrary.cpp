/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmldebugginglibrary.h"

#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <projectexplorer/project.h>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

namespace Qt4ProjectManager {


QString QmlDebuggingLibrary::libraryByInstallData(const QString &qtInstallData, bool debugBuild)
{
    if (!Core::ICore::instance())
        return QString();

    const QStringList directories = installDirectories(qtInstallData);

    QStringList binFilenames;
    if (debugBuild) {
        binFilenames << QLatin1String("QmlJSDebuggerd.lib");
        binFilenames << QLatin1String("libQmlJSDebuggerd.a"); // mingw
    } else {
        binFilenames << QLatin1String("QmlJSDebugger.lib");
    }
    binFilenames << QLatin1String("libQmlJSDebugger.a");

    return byInstallDataHelper(sourcePath(), sourceFileNames(), directories, binFilenames);
}

bool QmlDebuggingLibrary::canBuild(const QtVersion *qtVersion)
{
    return qtVersion->qtVersion() >=  QtVersionNumber(4, 7, 1);
}

bool  QmlDebuggingLibrary::build(const QString &directory, const QString &makeCommand,
                             const QString &qmakeCommand, const QString &mkspec,
                             const Utils::Environment &env, const QString &targetMode,
                             const QStringList &qmakeArguments, QString *output,  QString *errorMessage)
{
    return buildHelper(QCoreApplication::translate("Qt4ProjectManager::QmlDebuggingLibrary", "Qml Debugging"),
                       QLatin1String("qmljsdebugger.pro"),
                       directory, makeCommand, qmakeCommand, mkspec, env, targetMode,
                       qmakeArguments, output, errorMessage);
}

static inline bool mkpath(const QString &targetDirectory, QString *errorMessage)
{
    if (!QDir().mkpath(targetDirectory)) {
        *errorMessage = QCoreApplication::translate("Qt4ProjectManager::QmlDebuggingLibrary", "The target directory %1 could not be created.").arg(targetDirectory);
        return false;
    }
    return true;
}

QString QmlDebuggingLibrary::copy(const QString &qtInstallData, QString *errorMessage)
{
    const QStringList directories = QmlDebuggingLibrary::installDirectories(qtInstallData);

    // Try to find a writeable directory.
    foreach (const QString &directory, directories) {
        if (!mkpath(directory, errorMessage)) {
            continue;
        } else {
            errorMessage->clear();
        }

        if (copyFiles(sourcePath(), sourceFileNames(),
                      directory, errorMessage))
        {
            errorMessage->clear();
            return directory;
        }
    }
    *errorMessage = QCoreApplication::translate("Qt4ProjectManager::QmlDebuggingLibrary",
                                                "Qml Debugging library could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

QStringList QmlDebuggingLibrary::recursiveFileList(const QDir &dir, const QString &prefix)
{
    QStringList files;

    QString _prefix = prefix;
    if (!_prefix.isEmpty() && !_prefix.endsWith('/')) {
        _prefix = _prefix + '/';
    }
    foreach (const QString &fileName, dir.entryList(QDir::Files)) {
        files << _prefix + fileName;
    }

    foreach (const QString &subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        files += recursiveFileList(QDir(dir.absoluteFilePath(subDir)), _prefix + subDir);
    }
    return files;
}

QStringList QmlDebuggingLibrary::installDirectories(const QString &qtInstallData)
{
    const QChar slash = QLatin1Char('/');
    const uint hash = qHash(qtInstallData);
    QStringList directories;
    directories
            << (qtInstallData + QLatin1String("/qtc-qmldbg/"))
            << QDir::cleanPath((QCoreApplication::applicationDirPath() + QLatin1String("/../qtc-qmldbg/") + QString::number(hash))) + slash
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QLatin1String("/qtc-qmldbg/") + QString::number(hash)) + slash;
    return directories;
}

QString QmlDebuggingLibrary::sourcePath()
{
    return Core::ICore::instance()->resourcePath() + QLatin1String("/qml/qmljsdebugger/");
}

QStringList QmlDebuggingLibrary::sourceFileNames()
{
    return recursiveFileList(QDir(sourcePath()));
}

} // namespace
