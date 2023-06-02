// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosprobe.h"

#include <utils/algorithm.h>
#include <utils/process.h>

#include <QFileInfo>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(probeLog, "qtc.ios.probe", QtWarningMsg)

using namespace Utils;

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
    Utils::Process selectedXcode;
    selectedXcode.setTimeoutS(5);
    selectedXcode.setCommand({"/usr/bin/xcode-select", {"--print-path"}});
    selectedXcode.runBlocking();
    if (selectedXcode.result() != ProcessResult::FinishedWithSuccess)
        qCWarning(probeLog)
                << QString::fromLatin1("Could not detect selected Xcode using xcode-select");
    else
        addDeveloperPath(selectedXcode.cleanedStdOut().trimmed());
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
    clangProfile.developerPath = Utils::FilePath::fromString(devPath);

    const QFileInfo clangCInfo = getClangInfo("clang");
    if (clangCInfo.exists())
        clangProfile.cCompilerPath = Utils::FilePath::fromFileInfo(clangCInfo);

    const QFileInfo clangCppInfo = getClangInfo("clang++");
    if (clangCppInfo.exists())
        clangProfile.cxxCompilerPath = Utils::FilePath::fromFileInfo(clangCppInfo);

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
        sdk.path = Utils::FilePath::fromString(devPath
                + QString(QLatin1String("/Platforms/%1.platform/Developer/SDKs/%1.sdk")).arg(
                    sdk.directoryName));
        sdk.architectures = sdkConfig.second;
        const QFileInfo sdkPathInfo(sdk.path.toString());
        if (sdkPathInfo.exists() && sdkPathInfo.isDir()) {
            clangProfile.sdks.push_back(sdk);
            allArchitectures += Utils::toSet(sdk.architectures);
        }
    }

    if (!clangProfile.cCompilerPath.isEmpty() || !clangProfile.cxxCompilerPath.isEmpty()) {
        for (const QString &arch : std::as_const(allArchitectures)) {
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

size_t qHash(const XcodePlatform &platform)
{
    return qHash(platform.developerPath);
}

size_t qHash(const XcodePlatform::ToolchainTarget &target)
{
    return qHash(target.name);
}

bool XcodePlatform::ToolchainTarget::operator==(const XcodePlatform::ToolchainTarget &other) const
{
    return architecture == other.architecture;
}

}
