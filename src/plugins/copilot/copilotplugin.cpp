// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotplugin.h"

#include "copilotclient.h"
#include "copilotoptionspage.h"
#include "copilotsettings.h"
#include "copilottr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/texteditor.h>

#include <QAction>

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

    QAction *action = new QAction(this);
    action->setText(Tr::tr("Request Copilot Suggestion"));
    action->setToolTip(Tr::tr("Request Copilot Suggestion at the current editors cursor position."));

    QObject::connect(action, &QAction::triggered, this, [this] {
        if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget()) {
            if (m_client->reachable())
                m_client->requestCompletions(editor);
        }
    });
    ActionManager::registerAction(action, "Copilot.RequestSuggestion");
}

void CopilotPlugin::extensionsInitialized()
{
    (void)CopilotOptionsPage::instance();
}

void CopilotPlugin::restartClient()
{
    LanguageClient::LanguageClientManager::shutdownClient(m_client);
    m_client = new CopilotClient(CopilotSettings::instance().nodeJsPath.filePath(),
                                 CopilotSettings::instance().distPath.filePath());
}

} // namespace Internal
} // namespace Copilot
