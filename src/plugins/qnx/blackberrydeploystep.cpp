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

#include "blackberrydeploystep.h"

#include "qnxconstants.h"
#include "blackberrydeployconfiguration.h"
#include "blackberrydeploystepconfigwidget.h"
#include "qnxutils.h"
#include "blackberrydeployinformation.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>

#include <QDir>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char DEPLOY_CMD[] = "blackberry-deploy";

int parseProgress(const QString &line)
{
    const QString startOfLine = QLatin1String("Info: Progress ");
    if (!line.startsWith(startOfLine))
        return -1;

    const int percentPos = line.indexOf(QLatin1Char('%'));
    const QString progressStr = line.mid(startOfLine.length(), percentPos - startOfLine.length());

    bool ok;
    const int progress = progressStr.toInt(&ok);
    if (!ok)
        return -1;

    return progress;
}
}

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

    QString deployCmd = target()->activeBuildConfiguration()->environment().searchInPath(QLatin1String(DEPLOY_CMD));
    if (deployCmd.isEmpty()) {
        raiseError(tr("Could not find deploy command '%1' in the build environment")
                       .arg(QLatin1String(DEPLOY_CMD)));
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
        args << QnxUtils::addQuotes(QDir::toNativeSeparators(info.packagePath));

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
        if (!QFileInfo(info.packagePath).exists()) {
            raiseError(tr("Package '%1' does not exist. Create the package first.").arg(info.packagePath));
            fi.reportResult(false);
            return;
        }
    }

    BlackBerryAbstractDeployStep::run(fi);
}

void BlackBerryDeployStep::cleanup()
{
}

void BlackBerryDeployStep::stdOutput(const QString &line)
{
    const int progress = parseProgress(line);
    if (progress > -1)
        reportProgress(progress);

    BlackBerryAbstractDeployStep::stdOutput(line);
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

void BlackBerryDeployStep::raiseError(const QString &errorMessage)
{
    emit addOutput(errorMessage, BuildStep::ErrorMessageOutput);
    emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error, errorMessage, Utils::FileName(), -1,
                      Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
}
