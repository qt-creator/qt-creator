/**************************************************************************
**
** Copyright (C) 2012, 2013 BlackBerry Limited. All rights reserved
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

#include <QDir>
#include <QDesktopServices>
#include <QDomDocument>

using namespace Qnx;
using namespace Qnx::Internal;

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

QList<Utils::EnvironmentItem> QnxUtils::qnxEnvironmentFromNdkFile(const QString &fileName)
{
    QList <Utils::EnvironmentItem> items;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return items;

    QTextStream str(&file);
    QMap<QString, QString> fileContent;
    while (!str.atEnd()) {
        QString line = str.readLine();
        if (!line.contains(QLatin1Char('=')))
            continue;

        int equalIndex = line.indexOf(QLatin1Char('='));
        QString var = line.left(equalIndex);
        //Remove set in front
        if (var.startsWith(QLatin1String("set ")))
            var = var.right(var.size() - 4);

        QString value = line.mid(equalIndex + 1);

        // BASE_DIR (and BASE_DIR_REPLACED in some recent internal versions) variable is
        // evaluated when souring the bbnk-env script
        // BASE_DIR="$( cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd )"
        // We already know the NDK path so we can set the variable value
        // TODO: Do not parse bbnk-env!
        if (var == QLatin1String("BASE_DIR") ||  var == QLatin1String("BASE_DIR_REPLACED"))
            value = QFileInfo(fileName).dir().absolutePath();

        if (Utils::HostOsInfo::isWindowsHost()) {
            QRegExp systemVarRegExp(QLatin1String("IF NOT DEFINED ([\\w\\d]+)\\s+set ([\\w\\d]+)=([\\w\\d]+)"));
            if (line.contains(systemVarRegExp)) {
                var = systemVarRegExp.cap(2);
                Utils::Environment sysEnv = Utils::Environment::systemEnvironment();
                QString sysVar = systemVarRegExp.cap(1);
                if (sysEnv.hasKey(sysVar))
                    value = sysEnv.value(sysVar);
                else
                    value = systemVarRegExp.cap(3);
            }
        } else if (Utils::HostOsInfo::isAnyUnixHost()) {
            QRegExp systemVarRegExp(QLatin1String("\\$\\{([\\w\\d]+):=([\\w\\d]+)\\}")); // to match e.g. "${QNX_HOST_VERSION:=10_0_9_52}"
            if (value.contains(systemVarRegExp)) {
                Utils::Environment sysEnv = Utils::Environment::systemEnvironment();
                QString sysVar = systemVarRegExp.cap(1);
                if (sysEnv.hasKey(sysVar))
                    value = sysEnv.value(sysVar);
                else
                    value = systemVarRegExp.cap(2);
            }
        }

        if (value.startsWith(QLatin1Char('"')))
            value = value.mid(1);
        if (value.endsWith(QLatin1Char('"')))
            value = value.left(value.size() - 1);

        fileContent[var] = value;
    }
    file.close();

    QMapIterator<QString, QString> it(fileContent);
    while (it.hasNext()) {
        it.next();
        QStringList values;
        if (Utils::HostOsInfo::isWindowsHost())
            values = it.value().split(QLatin1Char(';'));
        else if (Utils::HostOsInfo::isAnyUnixHost())
            values = it.value().split(QLatin1Char(':'));

        QString key = it.key();
        foreach (const QString &value, values) {
            const QString ownKeyAsWindowsVar = QLatin1Char('%') + key + QLatin1Char('%');
            const QString ownKeyAsUnixVar = QLatin1Char('$') + key;
            if (value != ownKeyAsUnixVar && value != ownKeyAsWindowsVar) { // to ignore e.g. PATH=$PATH
                QString val = value;
                if (val.contains(QLatin1Char('%')) || val.contains(QLatin1Char('$'))) {
                    QMapIterator<QString, QString> replaceIt(fileContent);
                    while (replaceIt.hasNext()) {
                        replaceIt.next();
                        const QString replaceKey = replaceIt.key();
                        if (replaceKey == key)
                            continue;

                        const QString keyAsWindowsVar = QLatin1Char('%') + replaceKey + QLatin1Char('%');
                        const QString keyAsUnixVar = QLatin1Char('$') + replaceKey;
                        if (val.contains(keyAsWindowsVar))
                            val.replace(keyAsWindowsVar, replaceIt.value());
                        if (val.contains(keyAsUnixVar))
                            val.replace(keyAsUnixVar, replaceIt.value());
                    }
                }

                // This variable will be properly set based on the qt version architecture
                if (key == QLatin1String("CPUVARDIR"))
                    continue;

                items.append(Utils::EnvironmentItem(key, val));
            }
        }
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

Utils::FileName QnxUtils::executableWithExtension(const Utils::FileName &fileName)
{
    Utils::FileName result = fileName;
    if (Utils::HostOsInfo::isWindowsHost())
        result.append(QLatin1String(".exe"));
    return result;
}

QString QnxUtils::dataDirPath()
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

QString QnxUtils::qConfigPath()
{
    if (Utils::HostOsInfo::isMacHost() || Utils::HostOsInfo::isWindowsHost()) {
        return dataDirPath() + QLatin1String("/BlackBerry Native SDK/qconfig");
    } else {
        return dataDirPath() + QLatin1String("/bbndk/qconfig");
    }
}

QString QnxUtils::defaultTargetVersion(const QString &ndkPath)
{
    foreach (const NdkInstallInformation &ndkInfo, installedNdks()) {
        if (!ndkInfo.path.compare(ndkPath, Utils::HostOsInfo::fileNameCaseSensitivity()))
            return ndkInfo.version;
    }

    return QString();
}

QList<NdkInstallInformation> QnxUtils::installedNdks()
{
    QList<NdkInstallInformation> ndkList;
    QString ndkConfigPath = qConfigPath();
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
            NdkInstallInformation ndkInfo;
            ndkInfo.path = childElt.firstChildElement(QLatin1String("base")).text();
            ndkInfo.name = childElt.firstChildElement(QLatin1String("name")).text();
            ndkInfo.host = childElt.firstChildElement(QLatin1String("host")).text();
            ndkInfo.target = childElt.firstChildElement(QLatin1String("target")).text();
            ndkInfo.version = childElt.firstChildElement(QLatin1String("version")).text();

            ndkList.append(ndkInfo);
        }
    }

    return ndkList;
}

QString QnxUtils::sdkInstallerPath(const QString &ndkPath)
{
    QString sdkinstallPath;
    if (Utils::HostOsInfo::isWindowsHost())
        sdkinstallPath = ndkPath + QLatin1String("/qde.exe");
    else
        sdkinstallPath = ndkPath + QLatin1String("/qde");

    if (QFileInfo(sdkinstallPath).exists())
        return sdkinstallPath;

    return QString();
}

// The resulting process when launching sdkinstall
QString QnxUtils::qdeInstallProcess(const QString &ndkPath, const QString &option, const QString &version)
{
    QString installerPath = sdkInstallerPath(ndkPath);
    if (ndkPath.isEmpty())
        return QString();

    return QString::fromLatin1("%1 -nosplash -application com.qnx.tools.ide.sdk.manager.core.SDKInstallerApplication "
                               "%2  %3 -vmargs -Dosgi.console=:none").arg(installerPath, option, version);
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

        environmentItems.append(Utils::EnvironmentItem(QLatin1String("PATH"), sdkPath + QLatin1String("/host/win32/x86/usr/bin")));

        // TODO:
        //environment.insert(QLatin1String("PATH"), QLatin1String("/etc/qnx/bin"));
    } else if (Utils::HostOsInfo::isAnyUnixHost()) {
        environmentItems.append(Utils::EnvironmentItem(QLatin1String("QNX_CONFIGURATION"), QLatin1String("/etc/qnx")));
        environmentItems.append(Utils::EnvironmentItem(QLatin1String(Constants::QNX_TARGET_KEY), sdkPath + QLatin1String("/target/qnx6")));
        environmentItems.append(Utils::EnvironmentItem(QLatin1String(Constants::QNX_HOST_KEY), sdkPath + QLatin1String("/host/linux/x86")));

        environmentItems.append(Utils::EnvironmentItem(QLatin1String("PATH"), sdkPath + QLatin1String("/host/linux/x86/usr/bin")));
        environmentItems.append(Utils::EnvironmentItem(QLatin1String("PATH"), QLatin1String("/etc/qnx/bin")));

        environmentItems.append(Utils::EnvironmentItem(QLatin1String("LD_LIBRARY_PATH"), sdkPath + QLatin1String("/host/linux/x86/usr/lib")));
    }

    environmentItems.append(Utils::EnvironmentItem(QLatin1String("QNX_JAVAHOME"), sdkPath + QLatin1String("/_jvm")));
    environmentItems.append(Utils::EnvironmentItem(QLatin1String("MAKEFLAGS"), QLatin1String("-I") + sdkPath + QLatin1String("/target/qnx6/usr/include")));

    return environmentItems;
}
