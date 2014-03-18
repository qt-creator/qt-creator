/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "winrtpackagedeploymentstep.h"
#include "winrtpackagedeploymentstepwidget.h"
#include "winrtconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using Utils::QtcProcess;

namespace WinRt {
namespace Internal {

WinRtPackageDeploymentStep::WinRtPackageDeploymentStep(BuildStepList *bsl)
    : AbstractProcessStep(bsl, Constants::WINRT_BUILD_STEP_DEPLOY)
{
    setDisplayName(tr("Run windeployqt"));
    m_args = defaultWinDeployQtArguments();
}

bool WinRtPackageDeploymentStep::init()
{
    Utils::FileName proFile = Utils::FileName::fromString(project()->projectFilePath());
    const QString targetPath
            = target()->applicationTargets().targetForProject(proFile).toString()
                + QLatin1String(".exe");
    // ### Actually, targetForProject is supposed to return the file path including the file
    // extension. Whenever this will eventually work, we have to remove the .exe suffix here.

    QString args = QtcProcess::quoteArg(QDir::toNativeSeparators(targetPath));
    args += QLatin1Char(' ') + m_args;

    ProcessParameters *params = processParameters();
    params->setCommand(QLatin1String("windeployqt.exe"));
    params->setArguments(args);
    params->setEnvironment(target()->activeBuildConfiguration()->environment());

    return AbstractProcessStep::init();
}

BuildStepConfigWidget *WinRtPackageDeploymentStep::createConfigWidget()
{
    return new WinRtPackageDeploymentStepWidget(this);
}

void WinRtPackageDeploymentStep::setWinDeployQtArguments(const QString &args)
{
    m_args = args;
}

QString WinRtPackageDeploymentStep::winDeployQtArguments() const
{
    return m_args;
}

QString WinRtPackageDeploymentStep::defaultWinDeployQtArguments() const
{
    QString args;
    QtcProcess::addArg(&args, QStringLiteral("--qmldir"));
    QtcProcess::addArg(&args, QDir::toNativeSeparators(project()->projectDirectory()));
    return args;
}

bool WinRtPackageDeploymentStep::fromMap(const QVariantMap &map)
{
    if (!AbstractProcessStep::fromMap(map))
        return false;
    QVariant v = map.value(QLatin1String(Constants::WINRT_BUILD_STEP_DEPLOY_ARGUMENTS));
    if (v.isValid())
        m_args = v.toString();
    return true;
}

QVariantMap WinRtPackageDeploymentStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();
    map.insert(QLatin1String(Constants::WINRT_BUILD_STEP_DEPLOY_ARGUMENTS), m_args);
    return map;
}

} // namespace Internal
} // namespace WinRt
