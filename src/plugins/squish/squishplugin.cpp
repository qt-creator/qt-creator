// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "squishplugin.h"

#include "objectsmapeditor.h"
#include "squishnavigationwidget.h"
#include "squishoutputpane.h"
#include "squishresultmodel.h"
#include "squishsettings.h"
#include "squishtesttreemodel.h"
#include "squishtools.h"
#include "squishtr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QMenu>
#include <QtPlugin>

using namespace Core;

namespace Squish {
namespace Internal {

class SquishPluginPrivate : public QObject
{
public:
    SquishPluginPrivate();
    ~SquishPluginPrivate();

    void initializeMenuEntries();

    SquishSettings m_squishSettings;
    SquishSettingsPage m_settingsPage{&m_squishSettings};
    SquishTestTreeModel m_treeModel;
    SquishNavigationWidgetFactory m_navigationWidgetFactory;
    ObjectsMapEditorFactory m_objectsMapEditorFactory;
    SquishOutputPane *m_outputPane = nullptr;
    SquishTools * m_squishTools = nullptr;
};

static SquishPluginPrivate *dd = nullptr;

SquishPluginPrivate::SquishPluginPrivate()
{
    qRegisterMetaType<SquishResultItem*>("SquishResultItem*");

    m_squishSettings.readSettings(ICore::settings());
    m_outputPane = SquishOutputPane::instance();
    m_squishTools = new SquishTools;
    initializeMenuEntries();
}

SquishPluginPrivate::~SquishPluginPrivate()
{
    delete m_outputPane;
    delete m_squishTools;
}

SquishPlugin::~SquishPlugin()
{
    delete dd;
    dd = nullptr;
}

SquishSettings *SquishPlugin::squishSettings()
{
    QTC_ASSERT(dd, return nullptr);
    return &dd->m_squishSettings;
}

void SquishPluginPrivate::initializeMenuEntries()
{
    ActionContainer *menu = ActionManager::createMenu("Squish.Menu");
    menu->menu()->setTitle(Tr::tr("&Squish"));
    menu->setOnAllDisabledBehavior(ActionContainer::Show);

    QAction *action = new QAction(Tr::tr("&Server Settings..."), this);
    Command *command = ActionManager::registerAction(action, "Squish.ServerSettings");
    menu->addAction(command);
    connect(action, &QAction::triggered, this, [] {
        SquishServerSettingsDialog dialog;
        dialog.exec();
    });

    ActionContainer *toolsMenu = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(menu);
}

bool SquishPlugin::initialize(const QStringList &, QString *)
{
    dd = new SquishPluginPrivate;
    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag SquishPlugin::aboutToShutdown()
{
    if (dd->m_squishTools) {
        if (dd->m_squishTools->shutdown())
            return SynchronousShutdown;
        connect(dd->m_squishTools, &SquishTools::shutdownFinished,
                this, &ExtensionSystem::IPlugin::asynchronousShutdownFinished);
        return AsynchronousShutdown;
    }
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace Squish
