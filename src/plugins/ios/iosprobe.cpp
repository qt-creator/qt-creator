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

#include "iosprobe.h"

#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QLoggingCategory>
#include <QProcess>

static Q_LOGGING_CATEGORY(probeLog, "qtc.ios.probe")

namespace Ios {

static QString defaultDeveloperPath = QLatin1String("/Applications/Xcode.app/Contents/Developer");

QMap<QString, XcodePlatform> XcodeProbe::detectPlatforms(const QString &devPath)
{
    XcodeProbe probe;
    probe.addDeveloperPath(devPath);
    probe.detectFirst();
    return probe.detectedPlatforms();
}

void XcodeProbe::addDeveloperPath(const QString &path)
{
    if (path.isEmpty())
        return;
    QFileInfo pInfo(path);
    if (!pInfo.exists() || !pInfo.isDir())
        return;
    if (m_developerPaths.contains(path))
        return;
    m_developerPaths.append(path);
    qCDebug(probeLog) << QString::fromLatin1("Added developer path %1").arg(path);
}

void XcodeProbe::detectDeveloperPaths()
{
    Utils::SynchronousProcess selectedXcode;
    selectedXcode.setTimeoutS(5);
    Utils::SynchronousProcessResponse response = selectedXcode.run(
                QLatin1String("/usr/bin/xcode-select"), QStringList("--print-path"));
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        qCWarning(probeLog)
                << QString::fromLatin1("Could not detect selected Xcode using xcode-select");
    else
        addDeveloperPath(response.stdOut().trimmed());
    addDeveloperPath(defaultDeveloperPath);
}

void XcodeProbe::setupDefaultToolchains(const QString &devPath)
{
    auto getClangInfo = [devPath](const QString &compiler) {
        QFileInfo compilerInfo(devPath
                                + QLatin1String("/Toolchains/XcodeDefault.xctoolchain/usr/bin/")
                                + compiler);
        if (!compilerInfo.exists())
            qCWarning(probeLog) << QString::fromLatin1("Default toolchain %1 not found.")
                                    .arg(compilerInfo.canonicalFilePath());
        return compilerInfo;
    };

    XcodePlatform clangProfile;
    clangProfile.developerPath = Utils::FileName::fromString(devPath);

    const QFileInfo clangCInfo = getClangInfo("clang");
    if (clangCInfo.exists())
        clangProfile.cCompilerPath = Utils::FileName(clangCInfo);

    const QFileInfo clangCppInfo = getClangInfo("clang++");
    if (clangCppInfo.exists())
        clangProfile.cxxCompilerPath = Utils::FileName(clangCppInfo);

    QSet<QString> allArchitectures;
    static const std::map<QString, QStringList> sdkConfigs {
        {QLatin1String("AppleTVOS"), QStringList("arm64")},
        {QLatin1String("AppleTVSimulator"), QStringList("x86_64")},
        {QLatin1String("iPhoneOS"), QStringList { QLatin1String("arm64"), QLatin1String("armv7") }},
        {QLatin1String("iPhoneSimulator"), QStringList { QLatin1String("x86_64"),
                        QLatin1String("i386") }},
        {QLatin1String("MacOSX"), QStringList { QLatin1String("x86_64"), QLatin1String("i386") }},
        {QLatin1String("WatchOS"), QStringList("armv7k")},
        {QLatin1String("WatchSimulator"), QStringList("i386")}
    };
    for (const auto &sdkConfig : sdkConfigs) {
        XcodePlatform::SDK sdk;
        sdk.directoryName = sdkConfig.first;
        sdk.path = Utils::FileName::fromString(devPath
                + QString(QLatin1String("/Platforms/%1.platform/Developer/SDKs/%1.sdk")).arg(
                    sdk.directoryName));
        sdk.architectures = sdkConfig.second;
        const QFileInfo sdkPathInfo(sdk.path.toString());
        if (sdkPathInfo.exists() && sdkPathInfo.isDir()) {
            clangProfile.sdks.push_back(sdk);
            allArchitectures += sdk.architectures.toSet();
        }
    }

    if (!clangProfile.cCompilerPath.isEmpty() || !clangProfile.cxxCompilerPath.isEmpty()) {
        for (const QString &arch : allArchitectures) {
            const QString clangFullName = QString(QLatin1String("Apple Clang (%1)")).arg(arch)
                    + ((devPath != defaultDeveloperPath)
                       ? QString(QLatin1String(" in %1")).arg(devPath)
                       : QString());

            XcodePlatform::ToolchainTarget target;
            target.name = clangFullName;
            target.architecture = arch;
            target.backendFlags = QStringList { QLatin1String("-arch"), arch };
            clangProfile.targets.push_back(target);
        }
    }

    m_platforms[devPath] = clangProfile;
}

void XcodeProbe::detectFirst()
{
    detectDeveloperPaths();
    if (!m_developerPaths.isEmpty())
        setupDefaultToolchains(m_developerPaths.first());
}

QMap<QString, XcodePlatform> XcodeProbe::detectedPlatforms()
{
    return m_platforms;
}

bool XcodePlatform::operator==(const XcodePlatform &other) const
{
    return developerPath == other.developerPath;
}

uint qHash(const XcodePlatform &platform)
{
    return qHash(platform.developerPath);
}

uint qHash(const XcodePlatform::ToolchainTarget &target)
{
    return qHash(target.name);
}

bool XcodePlatform::ToolchainTarget::operator==(const XcodePlatform::ToolchainTarget &other) const
{
    return architecture == other.architecture;
}

}
