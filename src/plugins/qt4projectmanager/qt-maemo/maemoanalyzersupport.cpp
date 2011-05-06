/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemoanalyzersupport.h"
#include "maemoglobal.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzerconstants.h>
#include <analyzerbase/analyzerruncontrol.h>

#include <QtCore/QDir>

using namespace Utils;
using namespace Analyzer;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

RunControl *MaemoAnalyzerSupport::createAnalyzerRunControl(MaemoRunConfiguration *runConfig)
{
    AnalyzerStartParameters params;

    const MaemoDeviceConfig::ConstPtr &devConf = runConfig->deviceConfig();
    params.debuggee = runConfig->remoteExecutableFilePath();
    params.debuggeeArgs = runConfig->arguments();
    params.analyzerCmdPrefix = MaemoGlobal::remoteCommandPrefix(devConf->osVersion(),
        devConf->sshParameters().userName, runConfig->remoteExecutableFilePath())
        + MaemoGlobal::remoteEnvironment(runConfig->userEnvironmentChanges());
    params.startMode = StartRemote;
    params.connParams = devConf->sshParameters();
    params.localMountDir = runConfig->localDirToMountForRemoteGdb();
    params.remoteMountPoint = runConfig->remoteProjectSourcesMountPoint();
    const QString execDirAbs
        = QDir::fromNativeSeparators(QFileInfo(runConfig->localExecutableFilePath()).path());
    const QString execDirRel
        = QDir(params.localMountDir).relativeFilePath(execDirAbs);
    params.remoteSourcesDir = QString(params.remoteMountPoint
        + QLatin1Char('/') + execDirRel).toUtf8();
    params.displayName = runConfig->displayName();

    return AnalyzerManager::instance()->createAnalyzer(params, runConfig);
}

} // namespace Internal
} // namespace Qt4ProjectManager
