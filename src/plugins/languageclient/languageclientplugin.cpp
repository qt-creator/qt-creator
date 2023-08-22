// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientplugin.h"

#include "client.h"
#include "languageclientmanager.h"
#include "languageclientsettings.h"
#include "languageclienttr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <projectexplorer/projectpanelfactory.h>

#include <QAction>
#include <QMenu>

namespace LanguageClient {

static LanguageClientPlugin *m_instance = nullptr;

LanguageClientPlugin::LanguageClientPlugin()
{
    m_instance = this;
    qRegisterMetaType<LanguageServerProtocol::JsonRpcMessage>();
}

LanguageClientPlugin::~LanguageClientPlugin()
{
    m_instance = nullptr;
}

LanguageClientPlugin *LanguageClientPlugin::instance()
{
    return m_instance;
}

void LanguageClientPlugin::initialize()
{
    using namespace Core;

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory;
    panelFactory->setPriority(35);
    panelFactory->setDisplayName(Tr::tr("Language Server"));
    panelFactory->setCreateWidgetFunction(
        [](ProjectExplorer::Project *project) { return new ProjectSettingsWidget(project); });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);

    LanguageClientManager::init();
    LanguageClientSettings::registerClientType({Constants::LANGUAGECLIENT_STDIO_SETTINGS_ID,
                                                Tr::tr("Generic StdIO Language Server"),
                                                []() { return new StdIOSettings; }});

    //register actions
    ActionContainer *toolsDebugContainer = ActionManager::actionContainer(
        Core::Constants::M_TOOLS_DEBUG);

    auto inspectAction = new QAction(Tr::tr("Inspect Language Clients..."), this);
    connect(inspectAction, &QAction::triggered, this, &LanguageClientManager::showInspector);
    toolsDebugContainer->addAction(
        ActionManager::registerAction(inspectAction, "LanguageClient.InspectLanguageClients"));
}

void LanguageClientPlugin::extensionsInitialized()
{
    LanguageClientSettings::init();
}

ExtensionSystem::IPlugin::ShutdownFlag LanguageClientPlugin::aboutToShutdown()
{
    LanguageClientManager::shutdown();
    if (LanguageClientManager::isShutdownFinished())
        return ExtensionSystem::IPlugin::SynchronousShutdown;
    QTC_ASSERT(LanguageClientManager::instance(),
               return ExtensionSystem::IPlugin::SynchronousShutdown);
    connect(LanguageClientManager::instance(), &LanguageClientManager::shutdownFinished,
            this, &ExtensionSystem::IPlugin::asynchronousShutdownFinished);
    return ExtensionSystem::IPlugin::AsynchronousShutdown;
}

} // namespace LanguageClient
