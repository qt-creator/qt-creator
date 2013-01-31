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

#include "qmlobservertool.h"

#include "qtsupportconstants.h"
#include "baseqtversion.h"
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <projectexplorer/project.h>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

namespace QtSupport {

static QStringList recursiveFileList(const QDir &dir, const QString &prefix)
{
    QStringList files;

    QString _prefix = prefix;
    if (!_prefix.isEmpty() && !_prefix.endsWith(QLatin1Char('/')))
        _prefix.append(QLatin1Char('/'));

    foreach (const QString &fileName, dir.entryList(QDir::Files))
        files << _prefix + fileName;

    foreach (const QString &subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        files += recursiveFileList(QDir(dir.absoluteFilePath(subDir)), _prefix + subDir);

    return files;
}

static QStringList installDirectories(const QString &qtInstallData)
{
    const QChar slash = QLatin1Char('/');
    const uint hash = qHash(qtInstallData);
    QStringList directories;
    directories
            << (qtInstallData + QLatin1String("/qtc-qmlobserver/"))
            << QDir::cleanPath((QCoreApplication::applicationDirPath() + QLatin1String("/../qtc-qmlobserver/") + QString::number(hash))) + slash
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QLatin1String("/qtc-qmlobserver/") + QString::number(hash)) + slash;
    return directories;
}

static QString sourcePath()
{
    return Core::ICore::resourcePath() + QLatin1String("/qml/qmlobserver/");
}

static QStringList sourceFileNames()
{
    return recursiveFileList(QDir(sourcePath()), QString());
}

static QStringList validBinaryFilenames()
{
    return QStringList()
            << QLatin1String("debug/qmlobserver.exe")
            << QLatin1String("qmlobserver.exe")
            << QLatin1String("qmlobserver")
            << QLatin1String("QMLObserver.app/Contents/MacOS/QMLObserver");
}

bool QmlObserverTool::canBuild(const BaseQtVersion *qtVersion, QString *reason)
{
    if (qtVersion->type() != QLatin1String(Constants::DESKTOPQT)
            && qtVersion->type() != QLatin1String(Constants::SIMULATORQT)) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlObserverTool", "Only available for Qt for Desktop or Qt for Qt Simulator.");
        return false;
    }

    if (qtVersion->qtVersion() < QtVersionNumber(4, 7, 1)) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlObserverTool", "Only available for Qt 4.7.1 or newer.");
        return false;
    }
    if (qtVersion->qtVersion() >= QtVersionNumber(4, 8, 0)) {
        if (reason)
            *reason = QCoreApplication::translate("Qt4ProjectManager::QmlObserverTool", "Not needed.");
        return false;
    }
    return true;
}

QString QmlObserverTool::toolByInstallData(const QString &qtInstallData)
{
    if (!Core::ICore::instance())
        return QString();

    const QStringList directories = installDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames();

    return byInstallDataHelper(sourcePath(), sourceFileNames(), directories, binFilenames, false);
}

QStringList QmlObserverTool::locationsByInstallData(const QString &qtInstallData)
{
    QStringList result;
    QFileInfo fileInfo;
    const QStringList binFilenames = validBinaryFilenames();
    foreach (const QString &directory, installDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(binFilenames, directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

bool  QmlObserverTool::build(BuildHelperArguments arguments, QString *log, QString *errorMessage)
{
    arguments.helperName = QCoreApplication::translate("Qt4ProjectManager::QmlObserverTool", "QMLObserver");
    arguments.proFilename = QLatin1String("qmlobserver.pro");

    return buildHelper(arguments, log, errorMessage);
}

static inline bool mkpath(const QString &targetDirectory, QString *errorMessage)
{
    if (!QDir().mkpath(targetDirectory)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::QmlObserverTool", "The target directory %1 could not be created.").arg(targetDirectory);
        return false;
    }
    return true;
}

QString QmlObserverTool::copy(const QString &qtInstallData, QString *errorMessage)
{
    const QStringList directories = installDirectories(qtInstallData);

    // Try to find a writable directory.
    foreach (const QString &directory, directories) {
        if (!mkpath(directory, errorMessage))
            continue;

        errorMessage->clear();

        if (copyFiles(sourcePath(), sourceFileNames(), directory, errorMessage)) {
            errorMessage->clear();
            return directory;
        }
    }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::QmlObserverTool",
                                                "QMLObserver could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

} // namespace
