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
    using namespace Layouting;

    Column mainCol;
    mainCol.setSpacing(8);

    // Container for server widgets
    auto *serverContainer = new QWidget(this);
    m_serverLayout = new QVBoxLayout(serverContainer);
    m_serverLayout->setContentsMargins(0, 0, 0, 0);
    m_serverLayout->setSpacing(8);

    // Add existing servers
    const QStringList names = McpServerConfigStore::savedServerNames();
    for (const QString &name : names)
        addServer(name);

    mainCol.addItem(serverContainer);

    // "Add server" button
    auto *addButton = new QPushButton(tr("Add MCP server…"), this);
    connect(addButton, &QPushButton::clicked, this, &McpSettingsTab::promptAddServer);

    mainCol.addItem(addButton);
    mainCol.addItem(Stretch(1));
    mainCol.attachTo(this);
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
    auto *widget = new McpServerSettingsWidget(serverName, this);
    connect(widget, &McpServerSettingsWidget::removeRequested, this, &McpSettingsTab::removeServer);

    m_serverLayout->addWidget(widget);
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
    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("Add MCP server"));
    dialog.setLabelText(tr("Server name (must be unique):"));
    dialog.setInputMode(QInputDialog::TextInput);
    if (auto *le = dialog.findChild<QLineEdit *>())
        le->setMaxLength(64);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = dialog.textValue();
    if (name.trimmed().isEmpty())
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
