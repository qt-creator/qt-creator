//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the  GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2buildconfiguration.h"
#include "b2buildstep.h"
#include "b2projectmanager.h"
#include "b2projectmanagerplugin.h"
#include "b2projectmanagerconstants.h"
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

namespace BoostBuildProjectManager {
namespace Internal {

BoostBuildPlugin::BoostBuildPlugin()
{
    // Create your members
}

BoostBuildPlugin::~BoostBuildPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool BoostBuildPlugin::initialize(QStringList const& arguments, QString* errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize function, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    QLatin1String const mimeTypes(":boostbuildproject/BoostBuildProjectManager.mimetypes.xml");
    Utils::MimeDatabase::addMimeTypes(mimeTypes);

    addAutoReleasedObject(new BuildStepFactory);
    addAutoReleasedObject(new BuildConfigurationFactory);
    //TODO addAutoReleasedObject(new RunConfigurationFactory);
    addAutoReleasedObject(new ProjectManager);

    return true;
}

void BoostBuildPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized function, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag BoostBuildPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace BoostBuildProjectManager
