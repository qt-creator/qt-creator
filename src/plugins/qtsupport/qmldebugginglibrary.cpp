/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmldebugginglibrary.h"

#include "baseqtversion.h"
#include "qtsupportconstants.h"
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <projectexplorer/project.h>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

namespace QtSupport {


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
    binFilenames << QLatin1String("QmlJSDebugger.prl"); // Symbian. Note that the actual lib is in EPOCROOT

    return byInstallDataHelper(sourcePath(), sourceFileNames(), directories, binFilenames, false);
}

bool QmlDebuggingLibrary::canBuild(const BaseQtVersion *qtVersion, QString *reason)
{
    if (qtVersion->qtVersion() < QtVersionNumber(4, 7, 1)) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlDebuggingLibrary", "Only available for Qt 4.7.1 or newer.");
        return false;
    }
    if (qtVersion->qtVersion() >= QtVersionNumber(4, 8, 0)) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlDebuggingLibrary", "Not needed.");
        return false;
    }
    return true;
}

bool  QmlDebuggingLibrary::build(BuildHelperArguments arguments, QString *log, QString *errorMessage)
{
    arguments.helperName = QCoreApplication::translate("Qt4ProjectManager::QmlDebuggingLibrary", "QML Debugging");
    arguments.proFilename = QLatin1String("qmljsdebugger.pro");
    return buildHelper(arguments, log, errorMessage);
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
        if (!mkpath(directory, errorMessage))
            continue;
        else
            errorMessage->clear();

        if (copyFiles(sourcePath(), sourceFileNames(),
                      directory, errorMessage))
        {
            errorMessage->clear();
            return directory;
        }
    }
    *errorMessage = QCoreApplication::translate("Qt4ProjectManager::QmlDebuggingLibrary",
                                                "QML Debugging library could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

QStringList QmlDebuggingLibrary::recursiveFileList(const QDir &dir, const QString &prefix)
{
    QStringList files;

    QString _prefix = prefix;
    if (!_prefix.isEmpty() && !_prefix.endsWith(QLatin1Char('/')))
        _prefix = _prefix + QLatin1Char('/');
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
    return Core::ICore::resourcePath() + QLatin1String("/qml/qmljsdebugger/");
}

QStringList QmlDebuggingLibrary::sourceFileNames()
{
    return recursiveFileList(QDir(sourcePath()));
}

} // namespace
