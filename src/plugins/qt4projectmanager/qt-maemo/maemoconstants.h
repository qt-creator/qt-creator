/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef MAEMOCONSTANTS_H
#define MAEMOCONSTANTS_H

#include <QtCore/QLatin1String>

namespace Qt4ProjectManager {
namespace Internal {

#define PREFIX "Qt4ProjectManager.MaemoRunConfiguration"

#ifdef Q_OS_WIN32
#define EXEC_SUFFIX ".exe"
#else
#define EXEC_SUFFIX ""
#endif

static const QLatin1String MAEMO_RC_ID(PREFIX);
static const QLatin1String MAEMO_RC_ID_PREFIX(PREFIX ".");

static const QLatin1String ArgumentsKey(PREFIX ".Arguments");
static const QLatin1String SimulatorPathKey(PREFIX ".Simulator");
static const QLatin1String DeviceIdKey(PREFIX ".DeviceId");
static const QLatin1String LastDeployedHostsKey(PREFIX ".LastDeployedHosts");
static const QLatin1String LastDeployedFilesKey(PREFIX ".LastDeployedFiles");
static const QLatin1String LastDeployedRemotePathsKey(PREFIX ".LastDeployedRemotePaths");
static const QLatin1String LastDeployedTimesKey(PREFIX ".LastDeployedTimes");
static const QLatin1String DeployToSysrootKey(PREFIX ".DeployToSysroot");
static const QLatin1String ProFileKey(PREFIX ".ProFile");
static const QLatin1String ExportedLocalDirsKey(PREFIX ".ExportedLocalDirs");
static const QLatin1String RemoteMountPointsKey(PREFIX ".RemoteMountPoints");
static const QLatin1String BaseEnvironmentBaseKey(PREFIX ".BaseEnvironmentBase");
static const QLatin1String UserEnvironmentChangesKey(PREFIX ".UserEnvironmentChanges");
static const QLatin1String UseRemoteGdbKey(PREFIX ".UseRemoteGdb");

} // namespace Internal

namespace Constants {
const char * const MAEMO_SETTINGS_CATEGORY = "X.Maemo";
const char * const MAEMO_SETTINGS_TR_CATEGORY = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Maemo");
const char * const MAEMO_SETTINGS_CATEGORY_ICON = ":/projectexplorer/images/MaemoDevice.png";
}

} // namespace Qt4ProjectManager

#endif  // MAEMOCONSTANTS_H
