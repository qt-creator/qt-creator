/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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

#include "blackberrycheckdevicestatusstep.h"

#include "blackberrycheckdevicestatusstepconfigwidget.h"
#include "blackberrydeviceinformation.h"
#include "qnxversionnumber.h"
#include "qnxconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/task.h>
#include <ssh/sshconnection.h>

#include <coreplugin/icore.h>

#include <qfileinfo.h>

#include <qmessagebox.h>

#include <qeventloop.h>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char RUNTIME_CHECK_ENABLED[]     =
        "Qnx.Internal.BlackBerryCheckDeviceStatusStep.RuntimeCheckEnabled";
const char DEBUG_TOKEN_CHECK_ENABLED[] =
        "Qnx.Internal.BlackBerryCheckDeviceStatusStep.DebugTokenCheckEnabled";
}

BlackBerryCheckDeviceStatusStep::BlackBerryCheckDeviceStatusStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id(Constants::QNX_CHECK_DEVICE_STATUS_BS_ID))
  , m_deviceInfo(0)
  , m_eventLoop(0)
  , m_runtimeCheckEnabled(true)
  , m_debugTokenCheckEnabled(true)
{
    setDisplayName(tr("Check Device Status"));
}

BlackBerryCheckDeviceStatusStep::BlackBerryCheckDeviceStatusStep(ProjectExplorer::BuildStepList *bsl,
                                                                 BlackBerryCheckDeviceStatusStep *bs) :
    ProjectExplorer::BuildStep(bsl, bs)
  , m_deviceInfo(0)
  , m_eventLoop(0)
  , m_runtimeCheckEnabled(true)
  , m_debugTokenCheckEnabled(true)
{
    setDisplayName(tr("Check Device Status"));
}

void BlackBerryCheckDeviceStatusStep::checkDeviceInfo(int status)
{
    if (!m_runtimeCheckEnabled && m_debugTokenCheckEnabled) {
        // Skip debug token check for internal non secure devices and simulators
        if (!m_deviceInfo->isProductionDevice() || m_deviceInfo->isSimulator()) {
            m_eventLoop->exit(true);
            return;
        }
    }

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

    if (m_debugTokenCheckEnabled && !m_deviceInfo->debugTokenValid()) {
        //: %1: Error message from BlackBerryDeviceInformation
        const QString errorMsg =
            tr("%1. Upload a valid debug token into the device.")
            .arg(m_deviceInfo->debugTokenValidationError());
        raiseError(errorMsg);
        m_eventLoop->exit(false);
        return;
    }

    if (m_runtimeCheckEnabled) {
        QnxVersionNumber deviceRuntimeVersion(m_deviceInfo->scmBundle());
        QFileInfo fi(target()->kit()->autoDetectionSource());

        if (deviceRuntimeVersion.isEmpty()) {
            // Skip the check if device runtime is not determined
            m_eventLoop->exit(true);
            raiseWarning(tr("Cannot determine device runtime version."));
            return;
        }

        QnxVersionNumber apiLevelVersion = QnxVersionNumber::fromNdkEnvFileName(fi.baseName());
        if (apiLevelVersion.isEmpty()) {
            // Skip the check if device API level version is not determined
            m_eventLoop->exit(true);
            raiseWarning(tr("Cannot determine API level version."));
            return;
        }

        bool ok = true;
        if (apiLevelVersion > deviceRuntimeVersion) {
            raiseError(tr("The device runtime version (%1) is inferior to the API level version (%2)")
                       .arg(deviceRuntimeVersion.toString(), apiLevelVersion.toString()));

            QMetaObject::invokeMethod(this, "handleVersionMismatch", Qt::BlockingQueuedConnection,
                                      Q_RETURN_ARG(bool, ok),
                                      Q_ARG(QString, deviceRuntimeVersion.toString()),
                                      Q_ARG(QString, apiLevelVersion.toString()));
        }

        m_eventLoop->exit(ok);
        return;
    }

    m_eventLoop->exit(true);
}

void BlackBerryCheckDeviceStatusStep::emitOutputInfo()
{
    emit addOutput(tr("Checking device status..."), BuildStep::MessageOutput);
}

void BlackBerryCheckDeviceStatusStep::enableDebugTokenCheck(bool enable)
{
    m_debugTokenCheckEnabled = enable;
}

void BlackBerryCheckDeviceStatusStep::enableRuntimeCheck(bool enable)
{
    m_runtimeCheckEnabled = enable;
}

bool BlackBerryCheckDeviceStatusStep::handleVersionMismatch(const QString &runtimeVersion, const QString &apiLevelVersion)
{
    // TODO: Check if a matching API level exists in the user configurations,
    // otherwise let the user download the matching device runtime.
    const QMessageBox::StandardButton answer = QMessageBox::question(Core::ICore::mainWindow(), tr("Confirmation"),
                                                                     tr("The device runtime version (%1) does not match the API level version (%2).\n"
                                                                        "Do you want to continue anyway?").arg(runtimeVersion, apiLevelVersion),
                                                                     QMessageBox::Yes | QMessageBox::No);
    return answer == QMessageBox::Yes;
}

bool BlackBerryCheckDeviceStatusStep::init()
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

void BlackBerryCheckDeviceStatusStep::run(QFutureInterface<bool> &fi)
{
    if (!m_runtimeCheckEnabled && !m_debugTokenCheckEnabled)
        return fi.reportResult(true);

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

ProjectExplorer::BuildStepConfigWidget *BlackBerryCheckDeviceStatusStep::createConfigWidget()
{
    return new BlackBerryCheckDeviceStatusStepConfigWidget(this);
}

void BlackBerryCheckDeviceStatusStep::raiseError(const QString &errorMessage)
{
    emit addOutput(errorMessage, BuildStep::ErrorMessageOutput);
    emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error, errorMessage, Utils::FileName(), -1,
                                       ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT));
}

void BlackBerryCheckDeviceStatusStep::raiseWarning(const QString &warningMessage)
{
    emit addOutput(warningMessage, BuildStep::ErrorOutput);
    emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Warning, warningMessage, Utils::FileName(), -1,
                                       ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT));
}


bool BlackBerryCheckDeviceStatusStep::fromMap(const QVariantMap &map)
{
    m_runtimeCheckEnabled = map.value(QLatin1String(RUNTIME_CHECK_ENABLED), false).toBool();
    m_debugTokenCheckEnabled = map.value(QLatin1String(DEBUG_TOKEN_CHECK_ENABLED), false).toBool();

    return BuildStep::fromMap(map);
}

QVariantMap BlackBerryCheckDeviceStatusStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    map.insert(QLatin1String(RUNTIME_CHECK_ENABLED), m_runtimeCheckEnabled);
    map.insert(QLatin1String(DEBUG_TOKEN_CHECK_ENABLED), m_debugTokenCheckEnabled);

    return map;
}

bool BlackBerryCheckDeviceStatusStep::debugTokenCheckEnabled() const
{
    return m_debugTokenCheckEnabled;
}

bool BlackBerryCheckDeviceStatusStep::runtimeCheckEnabled() const
{
    return m_runtimeCheckEnabled;
}
