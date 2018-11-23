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

namespace ProjectExplorer {
class Kit;
class Target;
}

namespace Utils { class FileName; }

namespace Android {

class AndroidQtSupport;

class ANDROID_EXPORT AndroidManager : public QObject
{
    Q_OBJECT

public:
    static QString packageName(ProjectExplorer::Target *target);
    static QString packageName(const Utils::FileName &manifestFile);
    static bool packageInstalled(const QString &deviceSerial,
                                 const QString &packageName);
    static void apkInfo(const Utils::FileName &apkPath,
                        QString *packageName = nullptr,
                        QVersionNumber *version = nullptr,
                        QString *activityPath = nullptr);
    static QString intentName(ProjectExplorer::Target *target);
    static QString activityName(ProjectExplorer::Target *target);

    static bool bundleQt(ProjectExplorer::Target *target);
    static QString deviceSerialNumber(ProjectExplorer::Target *target);
    static void setDeviceSerialNumber(ProjectExplorer::Target *target, const QString &deviceSerialNumber);

    static int deviceApiLevel(ProjectExplorer::Target *target);
    static void setDeviceApiLevel(ProjectExplorer::Target *target, int level);

    static QString buildTargetSDK(ProjectExplorer::Target *target);

    static bool signPackage(ProjectExplorer::Target *target);

    static int minimumSDK(ProjectExplorer::Target *target);
    static int minimumSDK(const ProjectExplorer::Kit *kit);
    static int minimumNDK(ProjectExplorer::Target *target);

    static QString targetArch(ProjectExplorer::Target *target);

    static Utils::FileName dirPath(const ProjectExplorer::Target *target);
    static Utils::FileName manifestPath(ProjectExplorer::Target *target);
    static Utils::FileName manifestSourcePath(ProjectExplorer::Target *target);
    static Utils::FileName defaultPropertiesPath(ProjectExplorer::Target *target);
    static Utils::FileName apkPath(const ProjectExplorer::Target *target);

    static QPair<int, int> apiLevelRange();
    static QString androidNameForApiLevel(int x);

    static void cleanLibsOnDevice(ProjectExplorer::Target *target);
    static void installQASIPackage(ProjectExplorer::Target *target, const QString &packagePath);

    static bool checkKeystorePassword(const QString &keystorePath, const QString &keystorePasswd);
    static bool checkCertificatePassword(const QString &keystorePath, const QString &keystorePasswd, const QString &alias, const QString &certificatePasswd);
    static bool checkCertificateExists(const QString &keystorePath, const QString &keystorePasswd,
                                       const QString &alias);
    static AndroidQtSupport *androidQtSupport(ProjectExplorer::Target *target);
    static bool updateGradleProperties(ProjectExplorer::Target *target);
    static int findApiLevel(const Utils::FileName &platformPath);

    static void runAdbCommandDetached(const QStringList &args);
    static bool runAdbCommand(const QStringList &args, QString *output = nullptr);
    static bool runAaptCommand(const QStringList &args, QString *output = nullptr);
};

} // namespace Android
