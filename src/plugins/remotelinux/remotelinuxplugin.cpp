/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "remotelinuxplugin.h"

#include "embeddedlinuxqtversionfactory.h"
#include "embeddedlinuxtargetfactory.h"
#include "deployablefile.h"
#include "genericlinuxdeviceconfigurationfactory.h"
#include "genericremotelinuxdeploystepfactory.h"
#include "linuxdeviceconfigurations.h"
#include "qt4projectmanager/qt4projectmanagerconstants.h"
#include "remotelinuxdeployconfigurationfactory.h"
#include "remotelinuxrunconfigurationfactory.h"
#include "remotelinuxruncontrolfactory.h"
#include "remotelinuxsettingspages.h"
#include "startgdbserverdialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <debugger/debuggerconstants.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QtPlugin>
#include <QtGui/QAction>

namespace RemoteLinux {
namespace Internal {

RemoteLinuxPlugin::RemoteLinuxPlugin()
{
    setObjectName(QLatin1String("RemoteLinuxPlugin"));
}

bool RemoteLinuxPlugin::initialize(const QStringList &arguments,
    QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    LinuxDeviceConfigurations::instance(this);

    addObject(this);
    addAutoReleasedObject(new LinuxDeviceConfigurationsSettingsPage);
    addAutoReleasedObject(new GenericLinuxDeviceConfigurationFactory);
    addAutoReleasedObject(new RemoteLinuxRunConfigurationFactory);
    addAutoReleasedObject(new RemoteLinuxRunControlFactory);
    addAutoReleasedObject(new RemoteLinuxDeployConfigurationFactory);
    addAutoReleasedObject(new GenericRemoteLinuxDeployStepFactory);

    addAutoReleasedObject(new EmbeddedLinuxTargetFactory);
    addAutoReleasedObject(new EmbeddedLinuxQtVersionFactory);

    qRegisterMetaType<RemoteLinux::DeployableFile>("RemoteLinux::DeployableFile");

    return true;
}

RemoteLinuxPlugin::~RemoteLinuxPlugin()
{
    removeObject(this);
}

void RemoteLinuxPlugin::extensionsInitialized()
{
    /*
    using namespace Core;
    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    ActionContainer *mstart =
        am->actionContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING);

    const Context globalcontext(Core::Constants::C_GLOBAL);

    QAction *act = 0;
    Command *cmd = 0;

    act = new QAction(tr("Start Remote Debug Server..."), 0);
    cmd = am->registerAction(act, "StartGdbServer", globalcontext);
    cmd->setDescription(tr("Start Gdbserver"));
    mstart->addAction(cmd, Debugger::Constants::G_MANUAL_REMOTE);
    connect(act, SIGNAL(triggered()), SLOT(startGdbServer()));

    act = new QAction(tr("Attach to Running Remote Process..."), 0);
    cmd = am->registerAction(act, "AttachRemoteProcess", globalcontext);
    cmd->setDescription(tr("Attach to Remote Process"));
    mstart->addAction(cmd, Debugger::Constants::G_AUTOMATIC_REMOTE);
    connect(act, SIGNAL(triggered()), SLOT(startGdbServer()));
    */
}

void RemoteLinuxPlugin::startGdbServer()
{
    StartGdbServerDialog dlg;
    int result = dlg.exec();
    if (result == QDialog::Rejected)
        return;
    dlg.startGdbServer();
}

} // namespace Internal
} // namespace RemoteLinux

Q_EXPORT_PLUGIN(RemoteLinux::Internal::RemoteLinuxPlugin)
