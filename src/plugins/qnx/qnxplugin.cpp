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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxplugin.h"

#include "qnxconstants.h"
#include "qnxattachdebugsupport.h"
#include "qnxdeviceconfigurationfactory.h"
#include "qnxruncontrolfactory.h"
#include "qnxdeploystepfactory.h"
#include "qnxdeployconfigurationfactory.h"
#include "qnxrunconfigurationfactory.h"
#include "qnxqtversionfactory.h"
#include "qnxsettingspage.h"
#include "qnxconfigurationmanager.h"
#include "qnxtoolchain.h"
#include "qnxattachdebugsupport.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/kitmanager.h>

#include <QAction>
#include <QtPlugin>

using namespace ProjectExplorer;
using namespace Qnx::Internal;

QnxPlugin::QnxPlugin() : m_debugSeparator(0) , m_attachToQnxApplication(0)
{ }

bool QnxPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Handles QNX
    addAutoReleasedObject(new QnxConfigurationManager);
    addAutoReleasedObject(new QnxQtVersionFactory);
    addAutoReleasedObject(new QnxDeviceConfigurationFactory);
    addAutoReleasedObject(new QnxRunControlFactory);
    addAutoReleasedObject(new QnxDeployStepFactory);
    addAutoReleasedObject(new QnxDeployConfigurationFactory);
    addAutoReleasedObject(new QnxRunConfigurationFactory);
    addAutoReleasedObject(new QnxSettingsPage);

    // Handle Qcc Compiler
    addAutoReleasedObject(new QnxToolChainFactory);

    return true;
}

void QnxPlugin::extensionsInitialized()
{
    // Debug support
    QnxAttachDebugSupport *debugSupport = new QnxAttachDebugSupport(this);

    m_attachToQnxApplication = new QAction(this);
    m_attachToQnxApplication->setText(tr("Attach to remote QNX application..."));
    connect(m_attachToQnxApplication, SIGNAL(triggered()), debugSupport, SLOT(showProcessesDialog()));

    Core::ActionContainer *mstart = Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING);
    mstart->appendGroup(Constants::QNX_DEBUGGING_GROUP);
    mstart->addSeparator(Core::Context(Core::Constants::C_GLOBAL), Constants::QNX_DEBUGGING_GROUP, &m_debugSeparator);

    Core::Command *cmd = Core::ActionManager::registerAction(m_attachToQnxApplication, "Debugger.AttachToQnxApplication");
    mstart->addAction(cmd, Constants::QNX_DEBUGGING_GROUP);

    connect(KitManager::instance(), SIGNAL(kitsChanged()), this, SLOT(updateDebuggerActions()));
}

ExtensionSystem::IPlugin::ShutdownFlag QnxPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void QnxPlugin::updateDebuggerActions()
{
    bool hasValidQnxKit = false;

    KitMatcher matcher = DeviceTypeKitInformation::deviceTypeMatcher(Constants::QNX_QNX_OS_TYPE);
    foreach (Kit *qnxKit, KitManager::matchingKits(matcher)) {
        if (qnxKit->isValid() && !DeviceKitInformation::device(qnxKit).isNull()) {
            hasValidQnxKit = true;
            break;
        }
    }

    m_attachToQnxApplication->setVisible(hasValidQnxKit);
    m_debugSeparator->setVisible(hasValidQnxKit);
}
