/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
****************************************************************************/

#include "qmlprofilerextendedplugin.h"
#include "qmlprofilerextendedconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

#include "scenegraphtimelinemodel.h"
#include "pixmapcachemodel.h"

using namespace QmlProfilerExtended::Internal;

QmlProfilerExtendedPlugin::QmlProfilerExtendedPlugin()
{
    // Create your members
}

QmlProfilerExtendedPlugin::~QmlProfilerExtendedPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool QmlProfilerExtendedPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize method, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    addAutoReleasedObject(new PixmapCacheModel);
    addAutoReleasedObject(new SceneGraphTimelineModel);

    return true;
}

void QmlProfilerExtendedPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized method, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag QmlProfilerExtendedPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

void QmlProfilerExtendedPlugin::triggerAction()
{
    QMessageBox::information(Core::ICore::mainWindow(),
                             tr("Action triggered"),
                             tr("This is an action from QmlProfilerExtended."));
}

Q_EXPORT_PLUGIN2(QmlProfilerExtended, QmlProfilerExtendedPlugin)

