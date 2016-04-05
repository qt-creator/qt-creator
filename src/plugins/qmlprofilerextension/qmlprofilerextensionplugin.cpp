/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerextensionplugin.h"
#include "qmlprofilerextensionconstants.h"
#include <qmlprofiler/qmlprofilertimelinemodelfactory.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QDebug>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

#include "scenegraphtimelinemodel.h"
#include "pixmapcachemodel.h"
#include "memoryusagemodel.h"
#include "inputeventsmodel.h"
#include "debugmessagesmodel.h"
#include "flamegraphview.h"

using namespace QmlProfilerExtension::Internal;

class ModelFactory : public QmlProfiler::QmlProfilerTimelineModelFactory {
    Q_OBJECT
public:
    QList<QmlProfiler::QmlProfilerTimelineModel *> create(
            QmlProfiler::QmlProfilerModelManager *manager)
    {
        QList<QmlProfiler::QmlProfilerTimelineModel *> models;
        models << new PixmapCacheModel(manager, this)
               << new SceneGraphTimelineModel(manager, this)
               << new MemoryUsageModel(manager, this)
               << new InputEventsModel(manager, this)
               << new DebugMessagesModel(manager, this);
        return models;
    }
};

class ViewFactory : public QmlProfiler::QmlProfilerEventsViewFactory {
    Q_OBJECT
public:
    QList<QmlProfiler::QmlProfilerEventsView *> create(
            QWidget *parent, QmlProfiler::QmlProfilerModelManager *manager) override
    {
        QList<QmlProfiler::QmlProfilerEventsView *> views;
        views << new FlameGraphView(parent, manager);
        return views;
    }
};

QmlProfilerExtensionPlugin::QmlProfilerExtensionPlugin()
{
    // Create your members
}

QmlProfilerExtensionPlugin::~QmlProfilerExtensionPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool QmlProfilerExtensionPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize method, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    addAutoReleasedObject(new ModelFactory);
    addAutoReleasedObject(new ViewFactory);

    return true;
}

void QmlProfilerExtensionPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized method, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag QmlProfilerExtensionPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

void QmlProfilerExtensionPlugin::triggerAction()
{
    QMessageBox::information(Core::ICore::mainWindow(),
                             tr("Action Triggered"),
                             tr("This is an action from QML Profiler Extension."));
}

#include "qmlprofilerextensionplugin.moc"
