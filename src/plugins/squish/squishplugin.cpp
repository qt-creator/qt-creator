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

class SquishPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Squish.json")

private:
    void initialize() final
    {
        setupObjectsMapEditor();

        setupSquishOutputPane(this);
        setupSquishTools(this);

        setupSquishWizardPages();
        setupSquishNavigationWidgetFactory();

        qRegisterMetaType<SquishResultItem*>("SquishResultItem*");

        const Id menuId = "Squish.Menu";
        MenuBuilder(menuId)
            .setTitle(Tr::tr("&Squish"))
            .setOnAllDisabledBehavior(ActionContainer::Show)
            .addToContainer(Core::Constants::M_TOOLS);

        ActionBuilder(this, "Squish.ServerSettings")
            .setText(Tr::tr("&Server Settings..."))
            .addToContainer(menuId)
            .addOnTriggered(this, [] {
                if (!settings().squishPath().exists()) {
                    SquishMessages::criticalMessage(
                        Tr::tr("Invalid Squish settings. Configure Squish installation path inside "
                               "Preferences... > Squish > General to use this wizard."));
                    return;
                }
                SquishServerSettingsDialog dialog;
                dialog.exec();
            });

        ProjectExplorer::JsonWizardFactory::addWizardPath(":/squish/wizard/");
    }

    bool initializeGlobalScripts()
    {
        // The code expects squishTestTreeModel to exist, so force creation now.
        (void) SquishTestTreeModel::instance();

        SquishFileHandler::instance()->setSharedFolders({});

        const FilePath squishserver = settings().squishPath().pathAppended("bin/squishserver")
                .withExecutableSuffix();
        if (!squishserver.isExecutableFile())
            return false;

        SquishTools::instance()->queryGlobalScripts([](const QString &output, const QString &error) {
            if (output.isEmpty() || !error.isEmpty())
                return; // ignore (for now?)

            // FIXME? comma, special characters in paths
            const Utils::FilePaths globalDirs = Utils::transform(
                        output.trimmed().split(',', Qt::SkipEmptyParts), &Utils::FilePath::fromUserInput);
            SquishFileHandler::instance()->setSharedFolders(globalDirs);
        });
        return true;
    }

    bool delayedInitialize() final
    {
        connect(&settings().squishPath, &BaseAspect::changed,
                this, &SquishPlugin::initializeGlobalScripts);

        return initializeGlobalScripts();
    }

    ShutdownFlag aboutToShutdown() final
    {
        if (SquishTools::instance()->shutdown())
            return SynchronousShutdown;
        connect(SquishTools::instance(), &SquishTools::shutdownFinished,
                this, &ExtensionSystem::IPlugin::asynchronousShutdownFinished);
        return AsynchronousShutdown;
    }
};

} // Squish::Internal

#include "squishplugin.moc"
