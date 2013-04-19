/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "vcprojectmanagerplugin.h"

#include "vcprojectmanager.h"
#include "vcprojectmanagerconstants.h"
#include "vcprojectbuildconfiguration.h"
#include "vcprojectbuildoptionspage.h"
#include "vcmakestep.h"

// TODO: clean up
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

namespace VcProjectManager {
namespace Internal {

VcProjectManagerPlugin::VcProjectManagerPlugin()
{
}

VcProjectManagerPlugin::~VcProjectManagerPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool VcProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize method, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":vcproject/VcProject.mimetypes.xml"), errorString))
        return false;

    VcProjectBuildOptionsPage *confPage = new VcProjectBuildOptionsPage;
    addAutoReleasedObject(confPage);
    addAutoReleasedObject(new VcManager(confPage));
    addAutoReleasedObject(new VcProjectBuildConfigurationFactory);
    addAutoReleasedObject(new VcMakeStepFactory);

    return true;
}

void VcProjectManagerPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized method, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag VcProjectManagerPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace VcProjectManager

Q_EXPORT_PLUGIN2(VcProjectManager, VcProjectManager::Internal::VcProjectManagerPlugin)

