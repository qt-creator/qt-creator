/**************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "blackberrycheckdebugtokenstep.h"

#include "blackberrycheckdebugtokenstepconfigwidget.h"
#include "blackberrydeviceinformation.h"
#include "qnxconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <ssh/sshconnection.h>

#include <qeventloop.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryCheckDebugTokenStep::BlackBerryCheckDebugTokenStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QNX_CHECK_DEBUG_TOKEN_BS_ID))
  , m_deviceInfo(0)
  , m_eventLoop(0)
{
    setDisplayName(tr("Check Debug Token"));
}

BlackBerryCheckDebugTokenStep::BlackBerryCheckDebugTokenStep(ProjectExplorer::BuildStepList *bsl, BlackBerryCheckDebugTokenStep *bs) :
    ProjectExplorer::BuildStep(bsl, bs)
  , m_deviceInfo(0)
  , m_eventLoop(0)
{
    setDisplayName(tr("Check Debug Token"));
}

void BlackBerryCheckDebugTokenStep::checkDeviceInfo(int status)
{
    // Skip debug token check for internal non secure devices and simulators
    if (m_deviceInfo->isProductionDevice() && !m_deviceInfo->isSimulator()) {
        if (status != BlackBerryDeviceInformation::Success) {
            switch (status) {
            case BlackBerryDeviceInformation::AuthenticationFailed:
                raiseError(tr("Authentication failed."));
                break;
            case BlackBerryDeviceInformation::NoRouteToHost:
                raiseError(tr("Cannot connect to device."));
                break;
            case BlackBerryDeviceInformation::DevelopmentModeDisabled:
                raiseError(tr("Device is not in the development mode."));
                break;
            case BlackBerryDeviceInformation::InferiorProcessTimedOut:
                raiseError(tr("Timeout querying device information."));
                break;
            case BlackBerryDeviceInformation::FailedToStartInferiorProcess:
                raiseError(tr("Failed to query device information."));
                break;
            case BlackBerryDeviceInformation::InferiorProcessCrashed:
                raiseError(tr("Process to query device information has crashed."));
                break;
            default:
                raiseError(tr("Cannot query device information."));
                break;
            }
            m_eventLoop->exit(false);
            return;
        }

        if (!m_deviceInfo->debugTokenValid()) {
            raiseError(m_deviceInfo->debugTokenValidationError());
            m_eventLoop->exit(false);
            return;
        }
    }

    m_eventLoop->exit(true);
}

void BlackBerryCheckDebugTokenStep::emitOutputInfo()
{
    emit addOutput(tr("Checking debug token..."), BuildStep::MessageOutput);
}

bool BlackBerryCheckDebugTokenStep::init()
{
    m_device = BlackBerryDeviceConfiguration::device(target()->kit());
    if (!m_device)
        return false;

    if (m_device->sshParameters().host.isEmpty()) {
        raiseError(tr("No hostname specified for the device"));
        return false;
    }

    return true;
}

void BlackBerryCheckDebugTokenStep::run(QFutureInterface<bool> &fi)
{
    m_eventLoop = new QEventLoop;
    m_deviceInfo = new BlackBerryDeviceInformation;

    connect(m_deviceInfo, SIGNAL(started()), this, SLOT(emitOutputInfo()));
    connect(m_deviceInfo, SIGNAL(finished(int)), this, SLOT(checkDeviceInfo(int)), Qt::DirectConnection);
    m_deviceInfo->setDeviceTarget(m_device->sshParameters().host, m_device->sshParameters().password);

    bool returnValue = m_eventLoop->exec();

    delete m_eventLoop;
    m_eventLoop = 0;

    delete m_deviceInfo;
    m_deviceInfo = 0;

    return fi.reportResult(returnValue);
}

ProjectExplorer::BuildStepConfigWidget *BlackBerryCheckDebugTokenStep::createConfigWidget()
{
    return new BlackBerryCheckDebugTokenConfigWidget();
}

void BlackBerryCheckDebugTokenStep::raiseError(const QString &errorMessage)
{
    emit addOutput(errorMessage, BuildStep::ErrorMessageOutput);
    emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error, errorMessage, Utils::FileName(), -1,
                                       ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT));
}
