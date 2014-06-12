/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxutils.h"
#include "qnxabstractqtversion.h"

#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QDesktopServices>
#include <QDomDocument>
#include <QProcess>
#include <QTemporaryFile>
#include <QApplication>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char *EVAL_ENV_VARS[] = {
    "QNX_TARGET", "QNX_HOST", "QNX_CONFIGURATION", "MAKEFLAGS", "LD_LIBRARY_PATH",
    "PATH", "QDE", "CPUVARDIR", "PYTHONPATH"
};
}

QString QnxUtils::addQuotes(const QString &string)
{
    return QLatin1Char('"') + string + QLatin1Char('"');
}

Qnx::QnxArchitecture QnxUtils::cpudirToArch(const QString &cpuDir)
{
    if (cpuDir == QLatin1String("x86"))
        return Qnx::X86;
    else if (cpuDir == QLatin1String("armle-v7"))
        return Qnx::ArmLeV7;
    else
        return Qnx::UnknownArch;
}

QStringList QnxUtils::searchPaths(QnxAbstractQtVersion *qtVersion)
{
    const QDir pluginDir(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS")));
    const QStringList pluginSubDirs = pluginDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QStringList searchPaths;

    Q_FOREACH (const QString &dir, pluginSubDirs) {
        searchPaths << qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS"))
                       + QLatin1Char('/') + dir;
    }

    searchPaths << qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS"));
    searchPaths << qtVersion->qnxTarget() + QLatin1Char('/') + qtVersion->archString().toLower()
                   + QLatin1String("/lib");
    searchPaths << qtVersion->qnxTarget() + QLatin1Char('/') + qtVersion->archString().toLower()
                   + QLatin1String("/usr/lib");

    return searchPaths;
}

QList<Utils::EnvironmentItem> QnxUtils::qnxEnvironmentFromEnvFile(const QString &fileName)
{
    QList <Utils::EnvironmentItem> items;

    if (!QFileInfo(fileName).exists())
        return items;

    const bool isWindows = Utils::HostOsInfo::isWindowsHost();

    // locking creating bbndk-env file wrapper script
    QTemporaryFile tmpFile(
            QDir::tempPath() + QDir::separator()
            + QLatin1String("bbndk-env-eval-XXXXXX") + QLatin1String(isWindows ? ".bat" : ".sh"));
    if (!tmpFile.open())
        return items;
    tmpFile.setTextModeEnabled(true);

    // writing content to wrapper script
    QTextStream fileContent(&tmpFile);
    if (isWindows)
        fileContent << QLatin1String("@echo off\n")
                    << QLatin1String("call ") << fileName << QLatin1Char('\n');
    else
        fileContent << QLatin1String("#!/bin/bash\n")
                    << QLatin1String(". ") << fileName << QLatin1Char('\n');
    QString linePattern = QString::fromLatin1(isWindows ? "echo %1=%%1%" : "echo %1=$%1");
    for (int i = 0, len = sizeof(EVAL_ENV_VARS) / sizeof(const char *); i < len; ++i)
        fileContent << linePattern.arg(QLatin1String(EVAL_ENV_VARS[i])) << QLatin1Char('\n');
    tmpFile.close();

    // running wrapper script
    QProcess process;
    if (isWindows)
        process.start(QLatin1String("cmd.exe"),
                QStringList() << QLatin1String("/C") << tmpFile.fileName());
    else
        process.start(QLatin1String("/bin/bash"),
                QStringList() << tmpFile.fileName());

    // waiting for finish
    QApplication::setOverrideCursor(Qt::BusyCursor);
    bool waitResult = process.waitForFinished(10000);
    QApplication::restoreOverrideCursor();
    if (!waitResult) {
        Utils::SynchronousProcess::stopProcess(process);
        return items;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return items;

    // parsing process output
    QTextStream str(&process);
    while (!str.atEnd()) {
        QString line = str.readLine();
        int equalIndex = line.indexOf(QLatin1Char('='));
        if (equalIndex < 0)
            continue;
        QString var = line.left(equalIndex);
        QString value = line.mid(equalIndex + 1);
        items.append(Utils::EnvironmentItem(var, value));
    }

    return items;
}

bool QnxUtils::isValidNdkPath(const QString &ndkPath)
{
    return (QFileInfo(envFilePath(ndkPath)).exists());
}

QString QnxUtils::envFilePath(const QString &ndkPath, const QString &targetVersion)
{
    QString envFile;
    if (Utils::HostOsInfo::isWindowsHost())
        envFile = ndkPath + QLatin1String("/bbndk-env.bat");
    else if (Utils::HostOsInfo::isAnyUnixHost())
        envFile = ndkPath + QLatin1String("/bbndk-env.sh");

    if (!QFileInfo(envFile).exists()) {
        QString version = targetVersion.isEmpty() ? defaultTargetVersion(ndkPath) : targetVersion;
        version = version.replace(QLatin1Char('.'), QLatin1Char('_'));
        if (Utils::HostOsInfo::isWindowsHost())
            envFile = ndkPath + QLatin1String("/bbndk-env_") + version + QLatin1String(".bat");
        else if (Utils::HostOsInfo::isAnyUnixHost())
            envFile = ndkPath + QLatin1String("/bbndk-env_") + version + QLatin1String(".sh");
    }
    return envFile;
}

QString QnxUtils::bbDataDirPath()
{
    const QString homeDir = QDir::homePath();

    if (Utils::HostOsInfo::isMacHost())
        return homeDir + QLatin1String("/Library/Research in Motion");

    if (Utils::HostOsInfo::isAnyUnixHost())
        return homeDir + QLatin1String("/.rim");

    if (Utils::HostOsInfo::isWindowsHost()) {
        // Get the proper storage location on Windows using QDesktopServices,
        // to not hardcode "AppData/Local", as it might refer to "AppData/Roaming".
        QString dataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
        dataDir = dataDir.left(dataDir.indexOf(QCoreApplication::organizationName()));
        dataDir.append(QLatin1String("Research in Motion"));
        return dataDir;
    }

    return QString();
}

QString QnxUtils::bbqConfigPath()
{
    if (Utils::HostOsInfo::isMacHost() || Utils::HostOsInfo::isWindowsHost())
        return bbDataDirPath() + QLatin1String("/BlackBerry Native SDK/qconfig");
    else
        return bbDataDirPath() + QLatin1String("/bbndk/qconfig");
}

QString QnxUtils::defaultTargetVersion(const QString &ndkPath)
{
    foreach (const ConfigInstallInformation &ndkInfo, installedConfigs()) {
        if (!ndkInfo.path.compare(ndkPath, Utils::HostOsInfo::fileNameCaseSensitivity()))
            return ndkInfo.version;
    }

    return QString();
}

QList<ConfigInstallInformation> QnxUtils::installedConfigs(const QString &configPath)
{
    QList<ConfigInstallInformation> ndkList;
    QString ndkConfigPath = configPath;
    if (ndkConfigPath.isEmpty())
        ndkConfigPath = bbqConfigPath();

    if (!QDir(ndkConfigPath).exists())
        return ndkList;

    QFileInfoList ndkfileList = QDir(ndkConfigPath).entryInfoList(QStringList() << QLatin1String("*.xml"),
                                                                  QDir::Files, QDir::Time);
    foreach (const QFileInfo &ndkFile, ndkfileList) {
        QFile xmlFile(ndkFile.absoluteFilePath());
        if (!xmlFile.open(QIODevice::ReadOnly))
            continue;

        QDomDocument doc;
        if (!doc.setContent(&xmlFile))  // Skip error message
            continue;

        QDomElement docElt = doc.documentElement();
        if (docElt.tagName() != QLatin1String("qnxSystemDefinition"))
            continue;

        QDomElement childElt = docElt.firstChildElement(QLatin1String("installation"));
        // The file contains only one installation node
        if (!childElt.isNull()) {
            // The file contains only one base node
            ConfigInstallInformation ndkInfo;
            ndkInfo.path = childElt.firstChildElement(QLatin1String("base")).text();
            ndkInfo.name = childElt.firstChildElement(QLatin1String("name")).text();
            ndkInfo.host = childElt.firstChildElement(QLatin1String("host")).text();
            ndkInfo.target = childElt.firstChildElement(QLatin1String("target")).text();
            ndkInfo.version = childElt.firstChildElement(QLatin1String("version")).text();
            ndkInfo.installationXmlFilePath = ndkFile.absoluteFilePath();

            ndkList.append(ndkInfo);
        }
    }

    return ndkList;
}

QString QnxUtils::sdkInstallerPath(const QString &ndkPath)
{
    QString sdkinstallPath = Utils::HostOsInfo::withExecutableSuffix(ndkPath + QLatin1String("/qde"));

    if (QFileInfo(sdkinstallPath).exists())
        return sdkinstallPath;

    return QString();
}

// The resulting process when launching sdkinstall
QString QnxUtils::qdeInstallProcess(const QString &ndkPath, const QString &target,
                                    const QString &option, const QString &version)
{
    QString installerPath = sdkInstallerPath(ndkPath);
    if (installerPath.isEmpty())
        return QString();

    const QDir pluginDir(ndkPath + QLatin1String("/plugins"));
    const QStringList installerPlugins = pluginDir.entryList(QStringList() << QLatin1String("com.qnx.tools.ide.sdk.installer.app_*.jar"));
    const QString installerApplication = installerPlugins.size() >= 1 ? QLatin1String("com.qnx.tools.ide.sdk.installer.app.SDKInstallerApplication")
                                                                      : QLatin1String("com.qnx.tools.ide.sdk.manager.core.SDKInstallerApplication");
    return QString::fromLatin1("%1 -nosplash -application %2 "
                               "%3 %4 %5 -vmargs -Dosgi.console=:none").arg(installerPath, installerApplication, target, option, version);
}

QList<Utils::EnvironmentItem> QnxUtils::qnxEnvironment(const QString &sdkPath)
{
    // Mimic what the SDP installer puts into the system environment

    QList<Utils::EnvironmentItem> environmentItems;

    if (Utils::HostOsInfo::isWindowsHost()) {
        // TODO:
        //environment.insert(QLatin1String("QNX_CONFIGURATION"), QLatin1String("/etc/qnx"));
        environmentItems.append(Utils::EnvironmentItem(QLatin1String(Constants::QNX_TARGET_KEY), sdkPath + QLatin1String("/target/qnx6")));
        environmentItems.append(Utils::EnvironmentItem(QLatin1String(Constants::QNX_HOST_KEY), sdkPath + QLatin1String("/host/win32/x86")));

        environmentItems.append(Utils::EnvironmentItem(QLatin1String("PATH"), sdkPath + QLatin1String("/host/win32/x86/usr/bin;%PATH%")));

        // TODO:
        //environment.insert(QLatin1String("PATH"), QLatin1String("/etc/qnx/bin"));
    } else if (Utils::HostOsInfo::isAnyUnixHost()) {
        environmentItems.append(Utils::EnvironmentItem(QLatin1String("QNX_CONFIGURATION"), QLatin1String("/etc/qnx")));
        environmentItems.append(Utils::EnvironmentItem(QLatin1String(Constants::QNX_TARGET_KEY), sdkPath + QLatin1String("/target/qnx6")));
        environmentItems.append(Utils::EnvironmentItem(QLatin1String(Constants::QNX_HOST_KEY), sdkPath + QLatin1String("/host/linux/x86")));


        environmentItems.append(Utils::EnvironmentItem(QLatin1String("PATH"), sdkPath + QLatin1String("/host/linux/x86/usr/bin:/etc/qnx/bin:${PATH}")));

        environmentItems.append(Utils::EnvironmentItem(QLatin1String("LD_LIBRARY_PATH"), sdkPath + QLatin1String("/host/linux/x86/usr/lib:${LD_LIBRARY_PATH}")));
    }

    environmentItems.append(Utils::EnvironmentItem(QLatin1String("QNX_JAVAHOME"), sdkPath + QLatin1String("/_jvm")));
    environmentItems.append(Utils::EnvironmentItem(QLatin1String("MAKEFLAGS"), QLatin1String("-I") + sdkPath + QLatin1String("/target/qnx6/usr/include")));

    return environmentItems;
}
