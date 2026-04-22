// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpsettings.h"

#include "aiassistantconstants.h"
#include "aiassistantview.h"
#include "mcpserverconfigstore.h"
#include "mcpserversettingswidget.h"

#include <qmldesignertr.h>

#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace QmlDesigner {

McpSettings::McpSettings()
{
    setId(Constants::mcpSettingsPageId);
    setDisplayName(Tr::tr("MCP Servers"));
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
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    // remove the default QScrollArea background color
    auto palette = m_scrollArea->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    m_scrollArea->setPalette(palette);

    auto *serverContainer = new QWidget(this);

    m_serverLayout = new QVBoxLayout(serverContainer);
    m_serverLayout->setContentsMargins(0, 0, 0, 0);
    m_serverLayout->setSpacing(8);

    const QStringList names = McpServerConfigStore::savedServerNames();
    for (const QString &name : names)
        addServer(name);

    m_serverLayout->addStretch(1);
    m_scrollArea->setWidget(serverContainer);
    mainLayout->addWidget(m_scrollArea);

    auto *addButton = new QPushButton(tr("Add MCP server…"), this);
    connect(addButton, &QPushButton::clicked, this, &McpSettingsTab::promptAddServer);

    mainLayout->addWidget(addButton);
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

    QTimer::singleShot(100, this, [this, widget]{
        m_scrollArea->ensureWidgetVisible(widget);
    });
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

    if (McpServerConfigStore::savedServerNames().contains(name, Qt::CaseInsensitive)) {
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
