// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchatwidget.h"
#include "acpclientconstants.h"
#include "acpclienttr.h"
#include "acpinspector.h"
#include "acpsettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/rightpane.h>

#include <texteditor/texteditor.h>

#include <extensionsystem/iplugin.h>

#include <utils/utilsicons.h>

#include <QAction>
#include <QLoggingCategory>
#include <QMenu>
#include <QToolButton>

static Q_LOGGING_CATEGORY(logPlugin, "qtc.acpclient.plugin", QtWarningMsg);

using namespace Core;

namespace AcpClient::Internal {

class AcpClientPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AcpClient.json")

public:
    AcpClientPlugin() = default;

    ~AcpClientPlugin() final
    {
        delete m_rightPaneChatWidget;
        delete m_inspector;
    }

    void initialize() final
    {
        qCDebug(logPlugin) << "ACP Client plugin initializing...";

        setupAcpSettings();

        m_inspector = new AcpInspector;

        // Create the ACP Client menu under Tools
        ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
        menu->menu()->setTitle(Tr::tr("ACP Client"));
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

        // Show Chat in right side panel action
        auto showSidePanelAction = new QAction(
            Utils::Icons::MESSAGE_TOOLBAR.icon(),
            Tr::tr("Show Agentic AI Chat in Side Panel"), this);
        Command *sidePanelCmd = ActionManager::registerAction(
            showSidePanelAction, Constants::SHOW_CHAT_SIDEPANEL_ACTION_ID);
        menu->addAction(sidePanelCmd);

        // Add a tool button to each text editor toolbar
        connect(EditorManager::instance(), &EditorManager::editorOpened,
                this, [](IEditor *editor) {
            auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
            if (!textEditor)
                return;
            auto button = new QToolButton;
            button->setIcon(Utils::Icons::MESSAGE_TOOLBAR.icon());
            button->setToolTip(Tr::tr("Show Agentic AI Chat in Side Panel"));
            QObject::connect(button, &QToolButton::clicked,
                             ActionManager::command(Constants::SHOW_CHAT_SIDEPANEL_ACTION_ID)->action(),
                             &QAction::trigger);
            textEditor->editorWidget()->insertExtraToolBarWidget(
                TextEditor::TextEditorWidget::Right, button);
        });

        connect(showSidePanelAction, &QAction::triggered, this, [this] {
            createRightPaneChatWidget();
            ModeManager::activateMode(Core::Constants::MODE_EDIT);
            RightPaneWidget::instance()->setWidget(m_rightPaneChatWidget);
            RightPaneWidget::instance()->setShown(true);
            m_rightPaneChatWidget->setFocus();
        });

        // Inspect action
        auto inspectAction = new QAction(Tr::tr("Inspect ACP Client..."), this);
        Command *inspectCmd = ActionManager::registerAction(inspectAction, Constants::INSPECT_ACTION_ID);
        menu->addAction(inspectCmd);

        connect(inspectAction, &QAction::triggered, this, [this] {
            m_inspector->show();
        });
    }

    void extensionsInitialized() final {}

    ShutdownFlag aboutToShutdown() final
    {
        return SynchronousShutdown;
    }

private:
    void createRightPaneChatWidget()
    {
        if (m_rightPaneChatWidget)
            return;
        m_rightPaneChatWidget = new AcpChatWidget;
        m_rightPaneChatWidget->setInspector(m_inspector);
    }

    AcpInspector *m_inspector = nullptr;
    AcpChatWidget *m_rightPaneChatWidget = nullptr;
};

} // namespace AcpClient::Internal

#include "acpclientplugin.moc"
