/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved
** Contact: KDAB (info@kdab.com)
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

#include "qnxutils.h"
#include "qnxqtversion.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QStandardPaths>
#include <QApplication>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

const char *EVAL_ENV_VARS[] = {
    "QNX_TARGET", "QNX_HOST", "QNX_CONFIGURATION", "QNX_CONFIGURATION_EXCLUSIVE",
    "MAKEFLAGS", "LD_LIBRARY_PATH", "PATH", "QDE", "CPUVARDIR", "PYTHONPATH"
};

QString QnxUtils::cpuDirFromAbi(const Abi &abi)
{
    if (abi.os() != Abi::OS::QnxOS)
        return QString();
    if (abi.architecture() == Abi::Architecture::ArmArchitecture)
        return QString::fromLatin1(abi.wordWidth() == 32 ? "armle-v7" : "aarch64le");
    if (abi.architecture() == Abi::Architecture::X86Architecture)
        return QString::fromLatin1(abi.wordWidth() == 32 ? "x86" : "x86_64");
    return QString();
}

QString QnxUtils::cpuDirShortDescription(const QString &cpuDir)
{
    if (cpuDir == "armle-v7")
        return QLatin1String("32-bit ARM");

    if (cpuDir == "aarch64le")
        return QLatin1String("64-bit ARM");

    if (cpuDir == "x86")
        return QLatin1String("32-bit x86");

    if (cpuDir == "x86_64")
        return QLatin1String("64-bit x86");

    return cpuDir;
}

EnvironmentItems QnxUtils::qnxEnvironmentFromEnvFile(const QString &fileName)
{
    EnvironmentItems items;

    if (!QFileInfo::exists(fileName))
        return items;

    const bool isWindows = HostOsInfo::isWindowsHost();

    // locking creating sdp-env file wrapper script
    TemporaryFile tmpFile("sdp-env-eval-XXXXXX" + QString::fromLatin1(isWindows ? ".bat" : ".sh"));
    if (!tmpFile.open())
        return items;
    tmpFile.setTextModeEnabled(true);

    // writing content to wrapper script
    QTextStream fileContent(&tmpFile);
    if (isWindows)
        fileContent << "@echo off\n"
                    << "call " << fileName << '\n';
    else
        fileContent << "#!/bin/bash\n"
                    << ". " << fileName << '\n';
    QString linePattern = QString::fromLatin1(isWindows ? "echo %1=%%1%" : "echo %1=$%1");
    for (int i = 0, len = sizeof(EVAL_ENV_VARS) / sizeof(const char *); i < len; ++i)
        fileContent << linePattern.arg(QLatin1String(EVAL_ENV_VARS[i])) << QLatin1Char('\n');
    tmpFile.close();

    // running wrapper script
    QtcProcess process;
    if (isWindows)
        process.setCommand({"cmd.exe", {"/C", tmpFile.fileName()}});
    else
        process.setCommand({"/bin/bash", {tmpFile.fileName()}});
    process.start();

    // waiting for finish
    QApplication::setOverrideCursor(Qt::BusyCursor);
    bool waitResult = process.waitForFinished(10000);
    QApplication::restoreOverrideCursor();
    if (!waitResult) {
        process.stopProcess();
        return items;
    }

    if (process.result() != QtcProcess::FinishedWithSuccess)
        return items;

    // parsing process output
    QTextStream str(process.ioDevice());
    while (!str.atEnd()) {
        QString line = str.readLine();
        int equalIndex = line.indexOf(QLatin1Char('='));
        if (equalIndex < 0)
            continue;
        QString var = line.left(equalIndex);
        QString value = line.mid(equalIndex + 1);
        items.append(EnvironmentItem(var, value));
    }

    return items;
}

QString QnxUtils::envFilePath(const QString &sdpPath)
{
    QDir sdp(sdpPath);
    QStringList entries;
    if (HostOsInfo::isWindowsHost())
        entries = sdp.entryList(QStringList(QLatin1String("*-env.bat")));
    else
        entries = sdp.entryList(QStringList(QLatin1String("*-env.sh")));

    if (!entries.isEmpty())
        return sdp.absoluteFilePath(entries.first());

    return QString();
}

QString QnxUtils::defaultTargetVersion(const QString &sdpPath)
{
    foreach (const ConfigInstallInformation &sdpInfo, installedConfigs()) {
        if (!sdpInfo.path.compare(sdpPath, HostOsInfo::fileNameCaseSensitivity()))
            return sdpInfo.version;
    }

    return QString();
}

QList<ConfigInstallInformation> QnxUtils::installedConfigs(const QString &configPath)
{
    QList<ConfigInstallInformation> sdpList;
    QString sdpConfigPath = configPath;

    if (!QDir(sdpConfigPath).exists())
        return sdpList;

    QFileInfoList sdpfileList =
            QDir(sdpConfigPath).entryInfoList(QStringList() << QLatin1String("*.xml"),
                                              QDir::Files, QDir::Time);
    foreach (const QFileInfo &sdpFile, sdpfileList) {
        QFile xmlFile(sdpFile.absoluteFilePath());
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
            ConfigInstallInformation sdpInfo;
            sdpInfo.path = childElt.firstChildElement(QLatin1String("base")).text();
            sdpInfo.name = childElt.firstChildElement(QLatin1String("name")).text();
            sdpInfo.host = childElt.firstChildElement(QLatin1String("host")).text();
            sdpInfo.target = childElt.firstChildElement(QLatin1String("target")).text();
            sdpInfo.version = childElt.firstChildElement(QLatin1String("version")).text();
            sdpInfo.installationXmlFilePath = sdpFile.absoluteFilePath();

            sdpList.append(sdpInfo);
        }
    }

    return sdpList;
}

EnvironmentItems QnxUtils::qnxEnvironment(const QString &sdpPath)
{
    return qnxEnvironmentFromEnvFile(envFilePath(sdpPath));
}

QList<QnxTarget> QnxUtils::findTargets(const FilePath &basePath)
{
    QList<QnxTarget> result;

    QDirIterator iterator(basePath.toString());
    while (iterator.hasNext()) {
        iterator.next();
        const FilePath libc = FilePath::fromString(iterator.filePath()).pathAppended("lib/libc.so");
        if (libc.exists()) {
            auto abis = Abi::abisOfBinary(libc);
            if (abis.isEmpty()) {
                qWarning() << libc << "has no ABIs ... discarded";
                continue;
            }

            if (abis.count() > 1)
                qWarning() << libc << "has more than one ABI ... processing all";

            FilePath path = FilePath::fromString(iterator.filePath());
            for (const Abi &abi : abis)
                result.append(QnxTarget(path, QnxUtils::convertAbi(abi)));
        }
    }

    return result;
}

Abi QnxUtils::convertAbi(const Abi &abi)
{
    if (abi.os() == Abi::LinuxOS && abi.osFlavor() == Abi::GenericFlavor) {
        return Abi(abi.architecture(),
                   Abi::QnxOS,
                   Abi::GenericFlavor,
                   abi.binaryFormat(),
                   abi.wordWidth());
    }
    return abi;
}

Abis QnxUtils::convertAbis(const Abis &abis)
{
    return Utils::transform(abis, &QnxUtils::convertAbi);
}

} // Internal
} // Qnx
