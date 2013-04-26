/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrycheckdevmodestep.h"

#include "blackberrycheckdevmodestepconfigwidget.h"
#include "blackberrydeviceconfiguration.h"
#include "qnxconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryCheckDevModeStep::BlackBerryCheckDevModeStep(ProjectExplorer::BuildStepList *bsl) :
    BlackBerryAbstractDeployStep(bsl, Core::Id(Constants::QNX_CHECK_DEVELOPMENT_MODE_BS_ID))
{
    setDisplayName(tr("Check Development Mode"));
}

BlackBerryCheckDevModeStep::BlackBerryCheckDevModeStep(ProjectExplorer::BuildStepList *bsl, BlackBerryCheckDevModeStep *bs) :
    BlackBerryAbstractDeployStep(bsl, bs)
{
    setDisplayName(tr("Check Development Mode"));
}

bool BlackBerryCheckDevModeStep::init()
{
    if (!BlackBerryAbstractDeployStep::init())
        return false;

    QString deployCmd = target()->activeBuildConfiguration()->environment().searchInPath(QLatin1String(Constants::QNX_BLACKBERRY_DEPLOY_CMD));
    if (deployCmd.isEmpty()) {
        raiseError(tr("Could not find command '%1' in the build environment")
                       .arg(QLatin1String(Constants::QNX_BLACKBERRY_DEPLOY_CMD)));
        return false;
    }

    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(target()->kit());
    QString deviceHost = device ? device->sshParameters().host : QString();
    if (deviceHost.isEmpty()) {
        raiseError(tr("No hostname specified for device"));
        return false;
    }

    QStringList args;
    args << QLatin1String("-listDeviceInfo");
    args << deviceHost;
    if (!device->sshParameters().password.isEmpty()) {
        args << QLatin1String("-password");
        args << device->sshParameters().password;
    }

    addCommand(deployCmd, args);

    return true;
}

ProjectExplorer::BuildStepConfigWidget *BlackBerryCheckDevModeStep::createConfigWidget()
{
    return new BlackBerryCheckDevModeStepConfigWidget();
}

QString BlackBerryCheckDevModeStep::password() const
{
    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(target()->kit());
    return device ? device->sshParameters().password : QString();
}

void BlackBerryCheckDevModeStep::processStarted(const ProjectExplorer::ProcessParameters &params)
{
    QString arguments = params.prettyArguments();
    if (!password().isEmpty()) {
        const QString passwordLine = QLatin1String(" -password ") + password();
        const QString hiddenPasswordLine = QLatin1String(" -password <hidden>");
        arguments.replace(passwordLine, hiddenPasswordLine);
    }

    emitOutputInfo(params, arguments);
}
