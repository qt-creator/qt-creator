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

#include <projectexplorer/projectpanelfactory.h>

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
    QAction *requestAction = new QAction(this);
    requestAction->setText(Tr::tr("Request Copilot Suggestion"));
    requestAction->setToolTip(
        Tr::tr("Request Copilot suggestion at the current editor's cursor position."));

    connect(requestAction, &QAction::triggered, this, [this] {
        if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget()) {
            if (m_client && m_client->reachable())
                m_client->requestCompletions(editor);
        }
    });

    ActionManager::registerAction(requestAction, Constants::COPILOT_REQUEST_SUGGESTION);

    QAction *nextSuggestionAction = new QAction(this);
    nextSuggestionAction->setText(Tr::tr("Show Next Copilot Suggestion"));
    nextSuggestionAction->setToolTip(Tr::tr(
        "Cycles through the received Copilot Suggestions showing the next available Suggestion."));

    connect(nextSuggestionAction, &QAction::triggered, this, [] {
        if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
            cycleSuggestion(editor, Next);
    });

    ActionManager::registerAction(nextSuggestionAction, Constants::COPILOT_NEXT_SUGGESTION);

    QAction *previousSuggestionAction = new QAction(this);
    previousSuggestionAction->setText(Tr::tr("Show Previous Copilot Suggestion"));
    previousSuggestionAction->setToolTip(Tr::tr("Cycles through the received Copilot Suggestions "
                                                "showing the previous available Suggestion."));

    connect(previousSuggestionAction, &QAction::triggered, this, [] {
        if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
            cycleSuggestion(editor, Previous);
    });

    ActionManager::registerAction(previousSuggestionAction, Constants::COPILOT_PREVIOUS_SUGGESTION);

    QAction *disableAction = new QAction(this);
    disableAction->setText(Tr::tr("Disable Copilot"));
    disableAction->setToolTip(Tr::tr("Disable Copilot."));
    connect(disableAction, &QAction::triggered, this, [] {
        settings().enableCopilot.setValue(true);
        settings().apply();
    });
    ActionManager::registerAction(disableAction, Constants::COPILOT_DISABLE);

    QAction *enableAction = new QAction(this);
    enableAction->setText(Tr::tr("Enable Copilot"));
    enableAction->setToolTip(Tr::tr("Enable Copilot."));
    connect(enableAction, &QAction::triggered, this, [] {
        settings().enableCopilot.setValue(false);
        settings().apply();
    });
    ActionManager::registerAction(enableAction, Constants::COPILOT_ENABLE);

    QAction *toggleAction = new QAction(this);
    toggleAction->setText(Tr::tr("Toggle Copilot"));
    toggleAction->setCheckable(true);
    toggleAction->setChecked(settings().enableCopilot());
    toggleAction->setIcon(COPILOT_ICON.icon());
    connect(toggleAction, &QAction::toggled, this, [](bool checked) {
        settings().enableCopilot.setValue(checked);
        settings().apply();
    });

    ActionManager::registerAction(toggleAction, Constants::COPILOT_TOGGLE);

    auto updateActions = [toggleAction, requestAction] {
        const bool enabled = settings().enableCopilot();
        toggleAction->setToolTip(enabled ? Tr::tr("Disable Copilot.") : Tr::tr("Enable Copilot."));
        toggleAction->setChecked(enabled);
        requestAction->setEnabled(enabled);
    };

    connect(&settings().enableCopilot, &BaseAspect::changed, this, updateActions);

    updateActions();

    auto toggleButton = new QToolButton;
    toggleButton->setDefaultAction(toggleAction);
    StatusBarManager::addStatusBarWidget(toggleButton, StatusBarManager::RightCorner);

    auto panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(1000);
    panelFactory->setDisplayName(Tr::tr("Copilot"));
    panelFactory->setCreateWidgetFunction(&Internal::createCopilotProjectPanel);
    ProjectPanelFactory::registerFactory(panelFactory);
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
