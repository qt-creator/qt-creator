/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmldumptool.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <utils/runextensions.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QHash>
#include <QStandardPaths>

namespace QtSupport {

static inline QStringList validBinaryFilenames(bool debugBuild)
{
    QStringList list = QStringList()
            << QLatin1String("qmldump.exe")
            << QLatin1String("qmldump")
            << QLatin1String("qmldump.app/Contents/MacOS/qmldump");
    if (debugBuild)
        list.prepend(QLatin1String("debug/qmldump.exe"));
    else
        list.prepend(QLatin1String("release/qmldump.exe"));
    return list;
}

static inline QStringList validPrebuiltFilenames(bool debugBuild)
{
    QStringList list = QStringList(QLatin1String("qmlplugindump"));
    list.append(QLatin1String("qmlplugindump.app/Contents/MacOS/qmlplugindump"));
    if (debugBuild)
        list.prepend(QLatin1String("qmlplugindumpd.exe"));
    else
        list.prepend(QLatin1String("qmlplugindump.exe"));
    return list;
}

static bool hasPrivateHeaders(const QString &qtInstallHeaders) {
    const QString header = qtInstallHeaders
            + QLatin1String("/QtDeclarative/private/qdeclarativemetatype_p.h");
    return QFile::exists(header);
}

bool QmlDumpTool::canBuild(const BaseQtVersion *qtVersion, QString *reason)
{
    const QString installHeaders = qtVersion->qmakeProperty("QT_INSTALL_HEADERS");

    if (qtVersion->type() != QLatin1String(Constants::DESKTOPQT)) {
        if (reason)
            *reason = QCoreApplication::translate("QmakeProjectManager::QmlDumpTool", "Only available for Qt for Desktop.");
        return false;
    }
    if (qtVersion->qtVersion() < QtVersionNumber(4, 7, 1)) {
        if (reason)
            *reason = QCoreApplication::translate("QmakeProjectManager::QmlDumpTool", "Only available for Qt 4.7.1 or newer.");
        return false;
    }
    if (qtVersion->qtVersion() >= QtVersionNumber(4, 8, 0)) {
        if (reason)
            *reason = QCoreApplication::translate("QmakeProjectManager::QmlDumpTool", "Not needed.");
        return false;
    }


    if (!hasPrivateHeaders(installHeaders)) {
        if (reason)
            *reason = QCoreApplication::translate("QmakeProjectManager::QmlDumpTool", "Private headers are missing for this Qt version.");
        return false;
    }
    return true;
}

QString QmlDumpTool::toolForVersion(BaseQtVersion *version, bool debugDump)
{
    if (version) {
        const QString qtInstallData = version->qmakeProperty("QT_INSTALL_DATA");
        const QString qtInstallBins = version->qmakeProperty("QT_INSTALL_BINS");
        const QString qtInstallHeaders = version->qmakeProperty("QT_INSTALL_HEADERS");
        return toolForQtPaths(qtInstallData, qtInstallBins, qtInstallHeaders, debugDump);
    }

    return QString();
}

static QString sourcePath()
{
    return Core::ICore::resourcePath() + QLatin1String("/qml/qmldump/");
}

static QStringList sourceFileNames()
{
    QStringList files;
    files << QLatin1String("main.cpp") << QLatin1String("qmldump.pro")
          << QLatin1String("qmlstreamwriter.cpp") << QLatin1String("qmlstreamwriter.h")
          << QLatin1String("LICENSE.LGPLv21") << QLatin1String("LICENSE.LGPLv3")
          << QLatin1String("LGPL_EXCEPTION.TXT");
    if (Utils::HostOsInfo::isMacHost())
        files << QLatin1String("Info.plist");
    return files;
}

QString QmlDumpTool::toolForQtPaths(const QString &qtInstallData,
                                       const QString &qtInstallBins,
                                       const QString &qtInstallHeaders,
                                       bool debugDump)
{
    if (!Core::ICore::instance())
        return QString();

    // check for prebuilt binary first
    QFileInfo fileInfo;
    if (getHelperFileInfoFor(validPrebuiltFilenames(debugDump), qtInstallBins + QLatin1Char('/'), &fileInfo))
        return fileInfo.absoluteFilePath();

    const QStringList directories = installDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames(debugDump);

    return byInstallDataHelper(sourcePath(), sourceFileNames(), directories, binFilenames,
                               !hasPrivateHeaders(qtInstallHeaders));
}

QStringList QmlDumpTool::locationsByInstallData(const QString &qtInstallData, bool debugDump)
{
    QStringList result;
    QFileInfo fileInfo;
    const QStringList binFilenames = validBinaryFilenames(debugDump);
    foreach (const QString &directory, installDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(binFilenames, directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

bool QmlDumpTool::build(BuildHelperArguments arguments, QString *log, QString *errorMessage)
{
    arguments.helperName = QCoreApplication::translate("QmakeProjectManager::QmlDumpTool", "qmldump");
    arguments.proFilename = QLatin1String("qmldump.pro");
    return buildHelper(arguments, log, errorMessage);
}

QString QmlDumpTool::copy(const QString &qtInstallData, QString *errorMessage)
{
    const QStringList directories = QmlDumpTool::installDirectories(qtInstallData);

    // Try to find a writeable directory.
    foreach (const QString &directory, directories) {
        if (copyFiles(sourcePath(), sourceFileNames(), directory, errorMessage))
            return directory;
    }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::QmlDumpTool",
                                                "qmldump could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

QStringList QmlDumpTool::installDirectories(const QString &qtInstallData)
{
    const QChar slash = QLatin1Char('/');
    const uint hash = qHash(qtInstallData);
    QStringList directories;
    directories
            << (qtInstallData + QLatin1String("/qtc-qmldump/"))
            << QDir::cleanPath((QCoreApplication::applicationDirPath() + QLatin1String("/../qtc-qmldump/") + QString::number(hash))) + slash
            << (QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/data/qtc-qmldump/") + QString::number(hash)) + slash;
    return directories;
}

void QmlDumpTool::pathAndEnvironment(BaseQtVersion *version,
                                     bool preferDebug, QString *dumperPath, Utils::Environment *env)
{
    if (version && !version->hasQmlDump())
        return;

    QString path;

    path = toolForVersion(version, preferDebug);
    if (path.isEmpty())
        path = toolForVersion(version, !preferDebug);

    if (!path.isEmpty()) {
        QFileInfo qmldumpFileInfo(path);
        if (!qmldumpFileInfo.exists()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: qmldump executable does not exist at" << path;
            path.clear();
        } else if (!qmldumpFileInfo.isFile()) {
            qWarning() << "QmlDumpTool::qmlDumpPath: " << path << " is not a file";
            path.clear();
        }
    }

    if (!path.isEmpty() && version && dumperPath && env) {
        *dumperPath = path;
        *env = version->qmlToolsEnvironment();
    }
}

} // namespace QtSupport

