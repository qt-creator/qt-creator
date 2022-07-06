/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishplugin.h"

#include "objectsmapeditor.h"
#include "squishnavigationwidget.h"
#include "squishoutputpane.h"
#include "squishsettings.h"
#include "squishtesttreemodel.h"
#include "squishtools.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/mimetypes/mimedatabase.h>

#include <QMenu>
#include <QtPlugin>

using namespace Squish::Internal;
using namespace Core;

static SquishPlugin *m_instance = nullptr;

SquishPlugin::SquishPlugin()
{
    m_instance = this;
}

SquishPlugin::~SquishPlugin()
{
    delete m_objectsMapEditorFactory;
    delete m_navigationWidgetFactory;
    delete m_outputPane;
    delete m_squishTools;
}

SquishPlugin *SquishPlugin::instance()
{
    return m_instance;
}

SquishSettings *SquishPlugin::squishSettings()
{
    return &m_squishSettings;
}

void SquishPlugin::initializeMenuEntries()
{
    ActionContainer *menu = ActionManager::createMenu("Squish.Menu");
    menu->menu()->setTitle(tr("&Squish"));
    menu->setOnAllDisabledBehavior(ActionContainer::Show);

    QAction *action = new QAction(tr("&Server Settings..."), this);
    Command *command = ActionManager::registerAction(action, "Squish.ServerSettings");
    menu->addAction(command);
    connect(action, &QAction::triggered, this, [this] {
        SquishServerSettingsDialog dialog;
        dialog.exec();
    });

    ActionContainer *toolsMenu = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(menu);
}

bool SquishPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    m_squishSettings.readSettings(ICore::settings());
    m_squishTools = new SquishTools;

    initializeMenuEntries();

    m_treeModel = new SquishTestTreeModel(this);

    m_navigationWidgetFactory = new SquishNavigationWidgetFactory;
    m_outputPane = SquishOutputPane::instance();
    m_objectsMapEditorFactory = new ObjectsMapEditorFactory;
    return true;
}

void SquishPlugin::extensionsInitialized() {}

ExtensionSystem::IPlugin::ShutdownFlag SquishPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}
