/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydeploystep.h"

#include "qnxconstants.h"
#include "blackberrydeployconfiguration.h"
#include "blackberrydeploystepconfigwidget.h"
#include "qnxutils.h"
#include "blackberrydeployinformation.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>

#include <QDir>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryDeployStep::BlackBerryDeployStep(ProjectExplorer::BuildStepList *bsl)
    : BlackBerryAbstractDeployStep(bsl, Core::Id(Constants::QNX_DEPLOY_PACKAGE_BS_ID))
{
    setDisplayName(tr("Deploy packages"));
}

BlackBerryDeployStep::BlackBerryDeployStep(ProjectExplorer::BuildStepList *bsl, BlackBerryDeployStep *bs)
    : BlackBerryAbstractDeployStep(bsl, bs)
{
    setDisplayName(tr("Deploy packages"));
}

BlackBerryDeployStep::~BlackBerryDeployStep()
{
}

bool BlackBerryDeployStep::init()
{
    if (!BlackBerryAbstractDeployStep::init())
        return false;

    QString deployCmd = target()->activeBuildConfiguration()->environment().searchInPath(QLatin1String(Constants::QNX_BLACKBERRY_DEPLOY_CMD));
    if (deployCmd.isEmpty()) {
        raiseError(tr("Could not find deploy command \"%1\" in the build environment")
                       .arg(QLatin1String(Constants::QNX_BLACKBERRY_DEPLOY_CMD)));
        return false;
    }

    if (deviceHost().isEmpty()) {
        raiseError(tr("No hostname specified for device"));
        return false;
    }

    BlackBerryDeployConfiguration *deployConfig = qobject_cast<BlackBerryDeployConfiguration *>(deployConfiguration());
    QTC_ASSERT(deployConfig, return false);

    QList<BarPackageDeployInformation> packagesToDeploy = deployConfig->deploymentInfo()->enabledPackages();
    if (packagesToDeploy.isEmpty()) {
        raiseError(tr("No packages enabled for deployment"));
        return false;
    }

    foreach (const BarPackageDeployInformation &info, packagesToDeploy) {
        QStringList args;
        args << QLatin1String("-installApp");
        args << QLatin1String("-device") << deviceHost();
        if (!password().isEmpty())
            args << QLatin1String("-password") << password();
        args << QnxUtils::addQuotes(QDir::toNativeSeparators(info.packagePath()));

        addCommand(deployCmd, args);
    }

    return true;
}

void BlackBerryDeployStep::run(QFutureInterface<bool> &fi)
{
    BlackBerryDeployConfiguration *deployConfig = qobject_cast<BlackBerryDeployConfiguration *>(deployConfiguration());
    QTC_ASSERT(deployConfig, return);

    QList<BarPackageDeployInformation> packagesToDeploy = deployConfig->deploymentInfo()->enabledPackages();
    foreach (const BarPackageDeployInformation &info, packagesToDeploy) {
        if (!QFileInfo(info.packagePath()).exists()) {
            raiseError(tr("Package \"%1\" does not exist. Create the package first.").arg(info.packagePath()));
            fi.reportResult(false);
            return;
        }
    }

    BlackBerryAbstractDeployStep::run(fi);
}

void BlackBerryDeployStep::cleanup()
{
}

void BlackBerryDeployStep::processStarted(const ProjectExplorer::ProcessParameters &params)
{
    QString arguments = params.prettyArguments();
    if (!password().isEmpty()) {
        const QString passwordLine = QLatin1String(" -password ") + password();
        const QString hiddenPasswordLine = QLatin1String(" -password <hidden>");
        arguments.replace(passwordLine, hiddenPasswordLine);
    }

    emitOutputInfo(params, arguments);
}

ProjectExplorer::BuildStepConfigWidget *BlackBerryDeployStep::createConfigWidget()
{
    return new BlackBerryDeployStepConfigWidget();
}

QString BlackBerryDeployStep::deviceHost() const
{
    BlackBerryDeviceConfiguration::ConstPtr device
            = BlackBerryDeviceConfiguration::device(target()->kit());
    if (device)
        return device->sshParameters().host;
    return QString();
}

QString BlackBerryDeployStep::password() const
{
    BlackBerryDeviceConfiguration::ConstPtr device
            = BlackBerryDeviceConfiguration::device(target()->kit());
    if (device)
        return device->sshParameters().password;
    return QString();
}
