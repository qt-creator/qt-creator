/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#pragma once

#include "android_global.h"

#include <QPair>
#include <QObject>
#include <QVersionNumber>

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
    const QString &stdOut() { return m_stdOut; }
    const QString &stdErr() { return m_stdErr; }
    const QString &exitMessage() { return m_exitMessage; }

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
    static bool packageInstalled(const QString &deviceSerial, const QString &packageName);
    static int packageVersionCode(const QString &deviceSerial, const QString &packageName);
    static void apkInfo(const Utils::FilePath &apkPath,
                        QString *packageName = nullptr,
                        int *version = nullptr,
                        QString *activityPath = nullptr);
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

    static QStringList applicationAbis(const ProjectExplorer::Target *target);
    static QString archTriplet(const QString &abi);

    static Utils::FilePath dirPath(const ProjectExplorer::Target *target);
    static Utils::FilePath manifestPath(const ProjectExplorer::Target *target);
    static void setManifestPath(ProjectExplorer::Target *target, const Utils::FilePath &path);
    static Utils::FilePath manifestSourcePath(const ProjectExplorer::Target *target);
    static Utils::FilePath defaultPropertiesPath(const ProjectExplorer::Target *target);
    static Utils::FilePath apkPath(const ProjectExplorer::Target *target);
    static bool matchedAbis(const QStringList &deviceAbis, const QStringList &appAbis);
    static QString devicePreferredAbi(const QStringList &deviceAbis, const QStringList &appAbis);
    static ProjectExplorer::Abi androidAbi2Abi(const QString &androidAbi);

    static QPair<int, int> apiLevelRange();
    static QString androidNameForApiLevel(int x);

    static void installQASIPackage(ProjectExplorer::Target *target, const QString &packagePath);

    static bool checkKeystorePassword(const QString &keystorePath, const QString &keystorePasswd);
    static bool checkCertificatePassword(const QString &keystorePath, const QString &keystorePasswd,
                                         const QString &alias, const QString &certificatePasswd);
    static bool checkCertificateExists(const QString &keystorePath, const QString &keystorePasswd,
                                       const QString &alias);
    static bool updateGradleProperties(ProjectExplorer::Target *target, const QString &buildKey);
    static int findApiLevel(const Utils::FilePath &platformPath);

    static QProcess *runAdbCommandDetached(const QStringList &args, QString *err = nullptr,
                                           bool deleteOnFinish = false);
    static SdkToolResult runAdbCommand(const QStringList &args, const QByteArray &writeData = {},
                                       int timeoutS = 30);
    static SdkToolResult runAaptCommand(const QStringList &args, int timeoutS = 30);

    static QJsonObject deploymentSettings(const ProjectExplorer::Target *target);
    static bool isQtCreatorGenerated(const Utils::FilePath &deploymentFile);

private:
    static SdkToolResult runCommand(const Utils::CommandLine &command,
                                    const QByteArray &writeData = {}, int timeoutS = 30);
};

} // namespace Android
