/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
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

#ifndef ANDROIDMANAGER_H
#define ANDROIDMANAGER_H

#include "android_global.h"
#include <utils/fileutils.h>

#include <QPair>
#include <QObject>
#include <QStringList>

namespace ProjectExplorer {
class Kit;
class Target;
}

namespace Android {

class AndroidQtSupport;

class ANDROID_EXPORT AndroidManager : public QObject
{
    Q_OBJECT

public:

    static bool supportsAndroid(const ProjectExplorer::Kit *kit);
    static bool supportsAndroid(const ProjectExplorer::Target *target);

    static QString packageName(ProjectExplorer::Target *target);

    static QString intentName(ProjectExplorer::Target *target);
    static QString activityName(ProjectExplorer::Target *target);

    static bool bundleQt(ProjectExplorer::Target *target);
    static bool useLocalLibs(ProjectExplorer::Target *target);
    static QString deviceSerialNumber(ProjectExplorer::Target *target);
    static void setDeviceSerialNumber(ProjectExplorer::Target *target, const QString &deviceSerialNumber);

    static QString buildTargetSDK(ProjectExplorer::Target *target);

    static bool signPackage(ProjectExplorer::Target *target);

    static int minimumSDK(ProjectExplorer::Target *target);

    static QString targetArch(ProjectExplorer::Target *target);

    static Utils::FileName dirPath(ProjectExplorer::Target *target);
    static Utils::FileName manifestPath(ProjectExplorer::Target *target);
    static Utils::FileName libsPath(ProjectExplorer::Target *target);
    static Utils::FileName defaultPropertiesPath(ProjectExplorer::Target *target);

    static Utils::FileName localLibsRulesFilePath(ProjectExplorer::Target *target);
    static QString loadLocalLibs(ProjectExplorer::Target *target, int apiLevel = -1);
    static QString loadLocalJars(ProjectExplorer::Target *target, int apiLevel = -1);
    static QString loadLocalJarsInitClasses(ProjectExplorer::Target *target, int apiLevel = -1);

    static QPair<int, int> apiLevelRange();
    static QString androidNameForApiLevel(int x);

    static QStringList qtLibs(ProjectExplorer::Target *target);
    static QStringList prebundledLibs(ProjectExplorer::Target *target);

    static void cleanLibsOnDevice(ProjectExplorer::Target *target);
    static void installQASIPackage(ProjectExplorer::Target *target, const QString &packagePath);

    static bool checkKeystorePassword(const QString &keystorePath, const QString &keystorePasswd);
    static bool checkCertificatePassword(const QString &keystorePath, const QString &keystorePasswd, const QString &alias, const QString &certificatePasswd);
    static bool checkForQt51Files(Utils::FileName fileName);
    static AndroidQtSupport *androidQtSupport(ProjectExplorer::Target *target);
    static bool useGradle(ProjectExplorer::Target *target);
    static bool updateGradleProperties(ProjectExplorer::Target *target);
};

} // namespace Android

#endif // ANDROIDMANAGER_H
