// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientplugin.h"

#include "client.h"
#include "languageclientmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

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

bool LanguageClientPlugin::initialize(const QStringList & /*arguments*/, QString * /*errorString*/)
{
    using namespace Core;

    LanguageClientManager::init();
    LanguageClientSettings::registerClientType({Constants::LANGUAGECLIENT_STDIO_SETTINGS_ID,
                                                tr("Generic StdIO Language Server"),
                                                []() { return new StdIOSettings; }});

    //register actions
    ActionContainer *toolsDebugContainer = ActionManager::actionContainer(
        Core::Constants::M_TOOLS_DEBUG);

    auto inspectAction = new QAction(tr("Inspect Language Clients..."), this);
    connect(inspectAction, &QAction::triggered, this, &LanguageClientManager::showInspector);
    toolsDebugContainer->addAction(
        ActionManager::registerAction(inspectAction, "LanguageClient.InspectLanguageClients"));

    return true;
}

void LanguageClientPlugin::extensionsInitialized()
{
    LanguageClientSettings::init();
}

ExtensionSystem::IPlugin::ShutdownFlag LanguageClientPlugin::aboutToShutdown()
{
    LanguageClientManager::shutdown();
    if (LanguageClientManager::clients().isEmpty())
        return ExtensionSystem::IPlugin::SynchronousShutdown;
    QTC_ASSERT(LanguageClientManager::instance(),
               return ExtensionSystem::IPlugin::SynchronousShutdown);
    connect(LanguageClientManager::instance(), &LanguageClientManager::shutdownFinished,
            this, &ExtensionSystem::IPlugin::asynchronousShutdownFinished, Qt::QueuedConnection);
    return ExtensionSystem::IPlugin::AsynchronousShutdown;
}

} // namespace LanguageClient
