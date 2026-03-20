// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpsettings.h"

#include "aiassistantconstants.h"
#include "aiassistantview.h"
#include "mcpserverconfigstore.h"
#include "mcpserversettingswidget.h"

#include <qmldesignertr.h>

#include <utils/layoutbuilder.h>

#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace QmlDesigner {

McpSettings::McpSettings()
{
    setId(Constants::mcpSettingsPageId);
    setDisplayName(Tr::tr("MCP Server Settings"));
    setCategory(Constants::aiAssistantSettingsPageCategory);
    setWidgetCreator([this] { return new McpSettingsTab(m_view); });

    McpServerConfigStore::loadDefaults();
}

McpSettings::~McpSettings() = default;

void McpSettings::setView(AiAssistantView *view)
{
    m_view = view;
}

QList<McpServerConfig> McpSettings::allEnabledConfigs(const QString &projectPath)
{
    QList<McpServerConfig> result;
    const QStringList names = McpServerConfigStore::savedServerNames();
    for (const QString &name : names) {
        McpServerConfigStore store(name);
        if (store.isEnabled())
            result.append(store.toMcpServerConfig(projectPath));
    }
    return result;
}

McpSettingsTab::McpSettingsTab(AiAssistantView *view)
    : Core::IOptionsPageWidget()
    , m_view(view)
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    m_serversContainer = new QWidget;
    scroll->setWidget(m_serversContainer);

    auto *containerLayout = new QVBoxLayout(m_serversContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(8);
    containerLayout->addStretch(1);   // pushes server widgets upward

    // Add existing servers
    const QStringList names = McpServerConfigStore::savedServerNames();
    for (const QString &name : names)
        addServer(name);

    // "Add server" button at the bottom of the outer layout
    auto *addButton = new QPushButton(tr("Add MCP server…"), this);
    connect(addButton, &QPushButton::clicked, this, &McpSettingsTab::promptAddServer);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->addWidget(scroll);
    outerLayout->addWidget(addButton);
}

McpSettingsTab::~McpSettingsTab() = default;

void McpSettingsTab::apply()
{
    bool changed = false;
    for (McpServerSettingsWidget *w : std::as_const(m_serverWidgets))
        changed |= w->save();

    if (changed && m_view)
        m_view->updateMcpConfig();
}

void McpSettingsTab::cancel()
{
    for (McpServerSettingsWidget *w : std::as_const(m_serverWidgets))
        w->load();
}

void McpSettingsTab::addServer(const QString &serverName)
{
    auto *widget = new McpServerSettingsWidget(serverName, m_serversContainer);
    connect(widget, &McpServerSettingsWidget::removeRequested,
            this, &McpSettingsTab::removeServer);

    // Insert before the stretch item (last item in container layout)
    auto *layout = qobject_cast<QVBoxLayout *>(m_serversContainer->layout());
    const int insertPos = layout->count() - 1; // before the trailing stretch
    layout->insertWidget(insertPos, widget);
    m_serverWidgets.append(widget);
}

void McpSettingsTab::removeServer(const QString &serverName)
{
    const auto answer = QMessageBox::question(
        this,
        tr("Remove MCP server"),
        tr("Remove the server '%1' from the configuration?").arg(serverName),
        QMessageBox::Yes | QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    // Remove widget
    for (int i = 0; i < m_serverWidgets.size(); ++i) {
        if (m_serverWidgets[i]->serverName() == serverName) {
            m_serverWidgets[i]->deleteLater();
            m_serverWidgets.removeAt(i);
            break;
        }
    }

    // Remove from persistent store
    McpServerConfigStore::remove(serverName);
}

void McpSettingsTab::promptAddServer()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        tr("Add MCP server"),
        tr("Server name (must be unique):"),
        QLineEdit::Normal,
        {},
        &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    if (McpServerConfigStore::savedServerNames().contains(name)) {
        QMessageBox::warning(this, tr("Add MCP server"),
                             tr("A server named '%1' already exists.").arg(name));
        return;
    }

    // Persist an empty (default Stdio) entry so the widget can load it
    McpServerConfigStore newStore(name);
    newStore.save(true, McpServerConfig::Stdio, {}, {}, {}, {}, {});

    addServer(name);
}

} // namespace QmlDesigner
