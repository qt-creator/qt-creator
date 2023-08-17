// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectsmapeditor.h"
#include "squishfilehandler.h"
#include "squishmessages.h"
#include "squishnavigationwidget.h"
#include "squishoutputpane.h"
#include "squishresultmodel.h"
#include "squishsettings.h"
#include "squishtesttreemodel.h"
#include "squishtools.h"
#include "squishtr.h"
#include "squishwizardpages.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/iplugin.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QMenu>

using namespace Core;
using namespace Utils;

namespace Squish::Internal {

class SquishPluginPrivate final : public QObject
{
public:
    SquishPluginPrivate();

    bool initializeGlobalScripts();

    SquishTestTreeModel m_treeModel;
    SquishNavigationWidgetFactory m_navigationWidgetFactory;
    ObjectsMapEditorFactory m_objectsMapEditorFactory;
    SquishOutputPane m_outputPane;
    SquishTools m_squishTools;

    SquishToolkitsPageFactory m_squishToolkitsPageFactory;
    SquishScriptLanguagePageFactory m_squishScriptLanguagePageFactory;
    SquishAUTPageFactory m_squishAUTPageFactory;
    SquishGeneratorFactory m_squishGeneratorFactory;
};

SquishPluginPrivate::SquishPluginPrivate()
{
    qRegisterMetaType<SquishResultItem*>("SquishResultItem*");

    ActionContainer *menu = ActionManager::createMenu("Squish.Menu");
    menu->menu()->setTitle(Tr::tr("&Squish"));
    menu->setOnAllDisabledBehavior(ActionContainer::Show);

    QAction *action = new QAction(Tr::tr("&Server Settings..."), this);
    Command *command = ActionManager::registerAction(action, "Squish.ServerSettings");
    menu->addAction(command);
    connect(action, &QAction::triggered, this, [] {
        if (!settings().squishPath().exists()) {
            SquishMessages::criticalMessage(Tr::tr("Invalid Squish settings. Configure Squish "
                                                   "installation path inside "
                                                   "Preferences... > Squish > General to use "
                                                   "this wizard."));
            return;
        }

        SquishServerSettingsDialog dialog;
        dialog.exec();
    });

    ActionContainer *toolsMenu = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(menu);
}

bool SquishPluginPrivate::initializeGlobalScripts()
{
    SquishFileHandler::instance()->setSharedFolders({});

    const FilePath squishserver = settings().squishPath().pathAppended("bin/squishserver")
            .withExecutableSuffix();
    if (!squishserver.isExecutableFile())
        return false;

    m_squishTools.queryGlobalScripts([](const QString &output, const QString &error) {
        if (output.isEmpty() || !error.isEmpty())
            return; // ignore (for now?)

        // FIXME? comma, special characters in paths
        const Utils::FilePaths globalDirs = Utils::transform(
                    output.trimmed().split(',', Qt::SkipEmptyParts), &Utils::FilePath::fromUserInput);
        SquishFileHandler::instance()->setSharedFolders(globalDirs);
    });
    return true;
}

class SquishPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Squish.json")

private:
    void initialize() final
    {
        d.reset(new SquishPluginPrivate);
        ProjectExplorer::JsonWizardFactory::addWizardPath(":/squish/wizard/");
    }

    bool delayedInitialize() final
    {
        connect(&settings().squishPath, &BaseAspect::changed,
                d.get(), &SquishPluginPrivate::initializeGlobalScripts);

        return d->initializeGlobalScripts();
    }

    ShutdownFlag aboutToShutdown() final
    {
        if (d->m_squishTools.shutdown())
            return SynchronousShutdown;
        connect(&d->m_squishTools, &SquishTools::shutdownFinished,
                this, &ExtensionSystem::IPlugin::asynchronousShutdownFinished);
        return AsynchronousShutdown;
    }

    std::unique_ptr<SquishPluginPrivate> d;
};

} // Squish::Internal

#include "squishplugin.moc"
