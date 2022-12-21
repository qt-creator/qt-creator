// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "android_global.h"

#include <QPair>
#include <QObject>
#include <QVersionNumber>

#include <qtsupport/baseqtversion.h>
#include <projectexplorer/abi.h>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class Target;
}

namespace Utils {
class CommandLine;
class FilePath;
}

namespace Android {

class SdkToolResult {
public:
    SdkToolResult() = default;
    bool success() const { return m_success; }
    const QString &stdOut() const { return m_stdOut; }
    const QString &stdErr() const { return m_stdErr; }
    const QString &exitMessage() const { return m_exitMessage; }

private:
    bool m_success = false;
    QString m_stdOut;
    QString m_stdErr;
    QString m_exitMessage;
    friend class AndroidManager;
};

class ANDROID_EXPORT AndroidManager : public QObject
{
    Q_OBJECT

public:
    static QString packageName(const ProjectExplorer::Target *target);
    static QString packageName(const Utils::FilePath &manifestFile);
    static QString activityName(const ProjectExplorer::Target *target);

    static QString deviceSerialNumber(const ProjectExplorer::Target *target);
    static void setDeviceSerialNumber(ProjectExplorer::Target *target, const QString &deviceSerialNumber);

    static QString apkDevicePreferredAbi(const ProjectExplorer::Target *target);
    static void setDeviceAbis(ProjectExplorer::Target *target, const QStringList &deviceAbis);

    static int deviceApiLevel(const ProjectExplorer::Target *target);
    static void setDeviceApiLevel(ProjectExplorer::Target *target, int level);

    static QString buildTargetSDK(const ProjectExplorer::Target *target);

    static int minimumSDK(const ProjectExplorer::Target *target);
    static int minimumSDK(const ProjectExplorer::Kit *kit);
    static int defaultMinimumSDK(const QtSupport::QtVersion *qtVersion);

    static QStringList applicationAbis(const ProjectExplorer::Target *target);
    static QString archTriplet(const QString &abi);

    static bool isQt5CmakeProject(const ProjectExplorer::Target *target);

    static Utils::FilePath androidBuildDirectory(const ProjectExplorer::Target *target);
    static Utils::FilePath buildDirectory(const ProjectExplorer::Target *target);
    static Utils::FilePath manifestPath(const ProjectExplorer::Target *target);
    static void setManifestPath(ProjectExplorer::Target *target, const Utils::FilePath &path);
    static Utils::FilePath manifestSourcePath(const ProjectExplorer::Target *target);
    static Utils::FilePath apkPath(const ProjectExplorer::Target *target);
    static bool matchedAbis(const QStringList &deviceAbis, const QStringList &appAbis);
    static QString devicePreferredAbi(const QStringList &deviceAbis, const QStringList &appAbis);
    static ProjectExplorer::Abi androidAbi2Abi(const QString &androidAbi);

    static QString androidNameForApiLevel(int x);

    static void installQASIPackage(ProjectExplorer::Target *target, const Utils::FilePath &packagePath);

    static bool checkKeystorePassword(const Utils::FilePath &keystorePath,
                                      const QString &keystorePasswd);
    static bool checkCertificatePassword(const Utils::FilePath &keystorePath,
                                         const QString &keystorePasswd,
                                         const QString &alias, const QString &certificatePasswd);
    static bool checkCertificateExists(const Utils::FilePath &keystorePath,
                                       const QString &keystorePasswd, const QString &alias);

    static QProcess *runAdbCommandDetached(const QStringList &args, QString *err = nullptr,
                                           bool deleteOnFinish = false);
    static SdkToolResult runAdbCommand(const QStringList &args, const QByteArray &writeData = {},
                                       int timeoutS = 30);

    static QJsonObject deploymentSettings(const ProjectExplorer::Target *target);
    static bool isQtCreatorGenerated(const Utils::FilePath &deploymentFile);

private:
    static SdkToolResult runCommand(const Utils::CommandLine &command,
                                    const QByteArray &writeData = {}, int timeoutS = 30);
};

} // namespace Android
