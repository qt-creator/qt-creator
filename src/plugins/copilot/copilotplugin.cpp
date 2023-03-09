// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotplugin.h"

#include "copilotclient.h"
#include "copilotoptionspage.h"
#include "copilotsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/texteditor.h>

using namespace Utils;
using namespace Core;

namespace Copilot {
namespace Internal {

void CopilotPlugin::initialize()
{
    CopilotSettings::instance().readSettings(ICore::settings());

    restartClient();

    connect(&CopilotSettings::instance(),
            &CopilotSettings::applied,
            this,
            &CopilotPlugin::restartClient);
}

void CopilotPlugin::extensionsInitialized()
{
    CopilotOptionsPage::instance().init();
}

void CopilotPlugin::restartClient()
{
    LanguageClient::LanguageClientManager::shutdownClient(m_client);
    m_client = new CopilotClient(CopilotSettings::instance().nodeJsPath.filePath(),
                                 CopilotSettings::instance().distPath.filePath());
}

} // namespace Internal
} // namespace Copilot
