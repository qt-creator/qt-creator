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
#include <QStringList>

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

    static bool supportsAndroid(const ProjectExplorer::Kit *kit);
    static bool supportsAndroid(const ProjectExplorer::Target *target);

    static QString packageName(ProjectExplorer::Target *target);
    static QString packageName(const Utils::FileName &manifestFile);

    static QString intentName(ProjectExplorer::Target *target);
    static QString activityName(ProjectExplorer::Target *target);

    static bool bundleQt(ProjectExplorer::Target *target);
    static QString deviceSerialNumber(ProjectExplorer::Target *target);
    static void setDeviceSerialNumber(ProjectExplorer::Target *target, const QString &deviceSerialNumber);

    static QString buildTargetSDK(ProjectExplorer::Target *target);

    static bool signPackage(ProjectExplorer::Target *target);

    static int minimumSDK(ProjectExplorer::Target *target);
    static int minimumSDK(const ProjectExplorer::Kit *kit);

    static QString targetArch(ProjectExplorer::Target *target);

    static Utils::FileName dirPath(ProjectExplorer::Target *target);
    static Utils::FileName manifestPath(ProjectExplorer::Target *target);
    static Utils::FileName manifestSourcePath(ProjectExplorer::Target *target);
    static Utils::FileName defaultPropertiesPath(ProjectExplorer::Target *target);

    static QPair<int, int> apiLevelRange();
    static QString androidNameForApiLevel(int x);

    static void cleanLibsOnDevice(ProjectExplorer::Target *target);
    static void installQASIPackage(ProjectExplorer::Target *target, const QString &packagePath);

    static bool checkKeystorePassword(const QString &keystorePath, const QString &keystorePasswd);
    static bool checkCertificatePassword(const QString &keystorePath, const QString &keystorePasswd, const QString &alias, const QString &certificatePasswd);
    static bool checkCertificateExists(const QString &keystorePath, const QString &keystorePasswd,
                                       const QString &alias);
    static bool checkForQt51Files(Utils::FileName fileName);
    static AndroidQtSupport *androidQtSupport(ProjectExplorer::Target *target);
    static bool updateGradleProperties(ProjectExplorer::Target *target);
    static int findApiLevel(const Utils::FileName &platformPath);
};

} // namespace Android
