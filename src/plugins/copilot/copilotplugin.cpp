// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotplugin.h"

#include "copilotclient.h"
#include "copilotoptionspage.h"
#include "copilotsettings.h"

#include <coreplugin/icore.h>

using namespace Utils;

namespace Copilot {
namespace Internal {

CopilotPlugin::~CopilotPlugin()
{
    if (m_client)
        m_client->shutdown();

    m_client = nullptr;
}

void CopilotPlugin::initialize()
{
    CopilotSettings::instance().readSettings(Core::ICore::settings());

    m_client = new CopilotClient();

    connect(m_client, &CopilotClient::finished, this, [this]() { m_client = nullptr; });

    connect(&CopilotSettings::instance(), &CopilotSettings::applied, this, [this]() {
        if (m_client)
            m_client->shutdown();
        m_client = nullptr;
        m_client = new CopilotClient();
    });
}

void CopilotPlugin::extensionsInitialized()
{
    CopilotOptionsPage::instance().init();
}

} // namespace Internal
} // namespace Copilot
