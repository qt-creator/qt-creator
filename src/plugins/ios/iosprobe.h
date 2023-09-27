// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QSharedPointer>
#include <QString>
#include <QStringList>

namespace Ios {

class XcodePlatform
{
public:
    class SDK
    {
    public:
        QString directoryName;
        Utils::FilePath path;
        QStringList architectures;
    };
    class ToolchainTarget
    {
    public:
        QString name;
        QString architecture;
        QStringList backendFlags;

        bool operator==(const ToolchainTarget &other) const;
    };
    Utils::FilePath developerPath;
    Utils::FilePath cxxCompilerPath;
    Utils::FilePath cCompilerPath;
    std::vector<ToolchainTarget> targets;
    std::vector<SDK> sdks;

    // ignores anything besides name
    bool operator==(const XcodePlatform &other) const;
};

size_t qHash(const XcodePlatform &platform);
size_t qHash(const XcodePlatform::ToolchainTarget &target);

class XcodeProbe
{
public:
    static Utils::FilePath sdkPath(const QString &devPath, const QString &platformName);
    static QMap<QString, XcodePlatform> detectPlatforms(const QString &devPath = QString());

private:
    void addDeveloperPath(const QString &path);
    void detectDeveloperPaths();
    void setupDefaultToolchains(const QString &devPath);
    void detectFirst();
    QMap<QString, XcodePlatform> detectedPlatforms();
private:
    QMap<QString, XcodePlatform> m_platforms;
    QStringList m_developerPaths;
};

} // namespace Ios
