// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copilotplugin.h"

#include "copilotclient.h"
#include "copilotconstants.h"
#include "copiloticons.h"
#include "copilotprojectpanel.h"
#include "copilotsettings.h"
#include "copilotsuggestion.h"
#include "copilottr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/statusbarmanager.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <QAction>
#include <QToolButton>

using namespace Utils;
using namespace Core;
using namespace ProjectExplorer;

namespace Copilot {
namespace Internal {

enum Direction { Previous, Next };

void cycleSuggestion(TextEditor::TextEditorWidget *editor, Direction direction)
{
    QTextBlock block = editor->textCursor().block();
    if (auto *suggestion = dynamic_cast<CopilotSuggestion *>(
            TextEditor::TextDocumentLayout::suggestion(block))) {
        int index = suggestion->currentCompletion();
        if (direction == Previous)
            --index;
        else
            ++index;
        if (index < 0)
            index = suggestion->completions().count() - 1;
        else if (index >= suggestion->completions().count())
            index = 0;
        suggestion->reset();
        editor->insertSuggestion(std::make_unique<CopilotSuggestion>(suggestion->completions(),
                                                                     editor->document(),
                                                                     index));
    }
}

void CopilotPlugin::initialize()
{
    ActionBuilder requestAction(this,  Constants::COPILOT_REQUEST_SUGGESTION);
    requestAction.setText(Tr::tr("Request Copilot Suggestion"));
    requestAction.setToolTip(Tr::tr(
        "Request Copilot suggestion at the current editor's cursor position."));
    requestAction.setOnTriggered(this, [this] {
        if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget()) {
            if (m_client && m_client->reachable())
                m_client->requestCompletions(editor);
        }
    });

    ActionBuilder nextSuggestionAction(this, Constants::COPILOT_NEXT_SUGGESTION);
    nextSuggestionAction.setText(Tr::tr("Show Next Copilot Suggestion"));
    nextSuggestionAction.setToolTip(Tr::tr(
        "Cycles through the received Copilot Suggestions showing the next available Suggestion."));
    nextSuggestionAction.setOnTriggered(this, [] {
        if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
            cycleSuggestion(editor, Next);
    });

    ActionBuilder previousSuggestionAction(this, Constants::COPILOT_PREVIOUS_SUGGESTION);
    previousSuggestionAction.setText(Tr::tr("Show Previous Copilot Suggestion"));
    previousSuggestionAction.setToolTip(Tr::tr("Cycles through the received Copilot Suggestions "
                                               "showing the previous available Suggestion."));
    previousSuggestionAction.setOnTriggered(this, [] {
        if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
            cycleSuggestion(editor, Previous);
    });

    ActionBuilder disableAction(this, Constants::COPILOT_DISABLE);
    disableAction.setText(Tr::tr("Disable Copilot"));
    disableAction.setToolTip(Tr::tr("Disable Copilot."));
    disableAction.setOnTriggered(this, [] {
        settings().enableCopilot.setValue(true);
        settings().apply();
    });

    ActionBuilder enableAction(this, Constants::COPILOT_ENABLE);
    enableAction.setText(Tr::tr("Enable Copilot"));
    enableAction.setToolTip(Tr::tr("Enable Copilot."));
    enableAction.setOnTriggered(this, [] {
        settings().enableCopilot.setValue(false);
        settings().apply();
    });

    ActionBuilder toggleAction(this, Constants::COPILOT_TOGGLE);
    toggleAction.setText(Tr::tr("Toggle Copilot"));
    toggleAction.setCheckable(true);
    toggleAction.setChecked(settings().enableCopilot());
    toggleAction.setIcon(COPILOT_ICON.icon());
    toggleAction.setOnTriggered(this, [](bool checked) {
        settings().enableCopilot.setValue(checked);
        settings().apply();
    });

    QAction *toggleAct = toggleAction.contextAction();
    QAction *requestAct = requestAction.contextAction();
    auto updateActions = [toggleAct, requestAct] {
        const bool enabled = settings().enableCopilot();
        toggleAct->setToolTip(enabled ? Tr::tr("Disable Copilot.") : Tr::tr("Enable Copilot."));
        toggleAct->setChecked(enabled);
        requestAct->setEnabled(enabled);
    };

    connect(&settings().enableCopilot, &BaseAspect::changed, this, updateActions);

    updateActions();

    auto toggleButton = new QToolButton;
    toggleButton->setDefaultAction(toggleAction.contextAction());
    StatusBarManager::addStatusBarWidget(toggleButton, StatusBarManager::RightCorner);

    setupCopilotProjectPanel();
}

bool CopilotPlugin::delayedInitialize()
{
    restartClient();

    connect(&settings(), &AspectContainer::applied, this, &CopilotPlugin::restartClient);

    return true;
}

void CopilotPlugin::restartClient()
{
    LanguageClient::LanguageClientManager::shutdownClient(m_client);

    if (!settings().nodeJsPath().isExecutableFile())
        return;
    m_client = new CopilotClient(settings().nodeJsPath(), settings().distPath());
}

ExtensionSystem::IPlugin::ShutdownFlag CopilotPlugin::aboutToShutdown()
{
    if (!m_client)
        return SynchronousShutdown;
    connect(m_client, &QObject::destroyed, this, &IPlugin::asynchronousShutdownFinished);
    return AsynchronousShutdown;
}

} // namespace Internal
} // namespace Copilot
