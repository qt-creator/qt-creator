// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <extensionsystem/iplugin.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <QAction>
#include <QToolButton>
#include <QPointer>

using namespace Utils;
using namespace Core;
using namespace ProjectExplorer;

namespace Copilot::Internal {

enum Direction { Previous, Next };

static void cycleSuggestion(TextEditor::TextEditorWidget *editor, Direction direction)
{
    QTextBlock block = editor->textCursor().block();
    if (auto suggestion = dynamic_cast<CopilotSuggestion *>(
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

class CopilotPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Copilot.json")

public:
    void initialize() final
    {
        ActionBuilder requestAction(this,  Constants::COPILOT_REQUEST_SUGGESTION);
        requestAction.setText(Tr::tr("Request Copilot Suggestion"));
        requestAction.setToolTip(Tr::tr(
            "Request Copilot suggestion at the current editor's cursor position."));
        requestAction.addOnTriggered(this, [this] {
            if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget()) {
                if (m_client && m_client->reachable())
                    m_client->requestCompletions(editor);
            }
        });

        ActionBuilder nextSuggestionAction(this, Constants::COPILOT_NEXT_SUGGESTION);
        nextSuggestionAction.setText(Tr::tr("Show Next Copilot Suggestion"));
        nextSuggestionAction.setToolTip(Tr::tr(
            "Cycles through the received Copilot Suggestions showing the next available Suggestion."));
        nextSuggestionAction.addOnTriggered(this, [] {
            if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
                cycleSuggestion(editor, Next);
        });

        ActionBuilder previousSuggestionAction(this, Constants::COPILOT_PREVIOUS_SUGGESTION);
        previousSuggestionAction.setText(Tr::tr("Show Previous Copilot Suggestion"));
        previousSuggestionAction.setToolTip(Tr::tr("Cycles through the received Copilot Suggestions "
                                                   "showing the previous available Suggestion."));
        previousSuggestionAction.addOnTriggered(this, [] {
            if (auto editor = TextEditor::TextEditorWidget::currentTextEditorWidget())
                cycleSuggestion(editor, Previous);
        });

        ActionBuilder disableAction(this, Constants::COPILOT_DISABLE);
        disableAction.setText(Tr::tr("Disable Copilot"));
        disableAction.setToolTip(Tr::tr("Disable Copilot."));
        disableAction.addOnTriggered(this, [] {
            settings().enableCopilot.setValue(true);
            settings().apply();
        });

        ActionBuilder enableAction(this, Constants::COPILOT_ENABLE);
        enableAction.setText(Tr::tr("Enable Copilot"));
        enableAction.setToolTip(Tr::tr("Enable Copilot."));
        enableAction.addOnTriggered(this, [] {
            settings().enableCopilot.setValue(false);
            settings().apply();
        });

        ActionBuilder toggleAction(this, Constants::COPILOT_TOGGLE);
        toggleAction.setText(Tr::tr("Toggle Copilot"));
        toggleAction.setCheckable(true);
        toggleAction.setChecked(settings().enableCopilot());
        toggleAction.setIcon(COPILOT_ICON.icon());
        toggleAction.addOnTriggered(this, [](bool checked) {
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

        settings().enableCopilot.addOnChanged(this, updateActions);

        updateActions();

        auto toggleButton = new QToolButton;
        toggleButton->setDefaultAction(toggleAction.contextAction());
        StatusBarManager::addStatusBarWidget(toggleButton, StatusBarManager::RightCorner);

        setupCopilotProjectPanel();
    }

    bool delayedInitialize() final
    {
        restartClient();

        connect(&settings(), &AspectContainer::applied, this, &CopilotPlugin::restartClient);

        return true;
    }

    void restartClient()
    {
        LanguageClient::LanguageClientManager::shutdownClient(m_client);

        if (!settings().nodeJsPath().isExecutableFile())
            return;
        m_client = new CopilotClient(settings().nodeJsPath(), settings().distPath());
    }

    ShutdownFlag aboutToShutdown() final
    {
        if (!m_client)
            return SynchronousShutdown;
        connect(m_client, &QObject::destroyed, this, &IPlugin::asynchronousShutdownFinished);
        return AsynchronousShutdown;
    }

private:
    QPointer<CopilotClient> m_client;
};

} // Copilot::Internal

#include "copilotplugin.moc"
