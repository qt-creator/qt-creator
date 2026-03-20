// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpserversettingswidget.h"

#include "stringlistwidget.h"

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolBar>

namespace QmlDesigner {

McpServerSettingsWidget::McpServerSettingsWidget(const QString &serverName, QWidget *parent)
    : QGroupBox(parent)
    , m_store(serverName)
    , m_transportCombo(Utils::makeUniqueObjectPtr<QComboBox>())
    , m_commandLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_workingDirLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_argsWidget(Utils::makeUniqueObjectPtr<StringListWidget>())
    , m_urlLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_bearerTokenLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_transportStack(Utils::makeUniqueObjectPtr<QStackedWidget>())
{
    setupUi();
    load();
}

McpServerSettingsWidget::~McpServerSettingsWidget() = default;

void McpServerSettingsWidget::load()
{
    setChecked(m_store.isEnabled());

    const int transportIndex = (m_store.transport() == McpServerConfig::Http) ? 1 : 0;
    m_transportCombo->setCurrentIndex(transportIndex);
    updateTransportPanelVisibility();

    // Stdio
    m_commandLineEdit->setText(m_store.command());
    m_workingDirLineEdit->setText(m_store.workingDir());
    m_argsWidget->setItems(m_store.args(), {});

    // HTTP
    m_urlLineEdit->setText(m_store.url());
    m_bearerTokenLineEdit->setText(m_store.bearerToken());
}

bool McpServerSettingsWidget::save()
{
    const McpServerConfig::Transport newTransport =
        (m_transportCombo->currentIndex() == 1) ? McpServerConfig::Http : McpServerConfig::Stdio;

    const bool changed = m_store.isEnabled() != isChecked()
                      || m_store.transport() != newTransport
                      || m_store.command() != m_commandLineEdit->text()
                      || m_store.workingDir() != m_workingDirLineEdit->text()
                      || m_store.args() != m_argsWidget->items()
                      || m_store.url() != m_urlLineEdit->text()
                      || m_store.bearerToken() != m_bearerTokenLineEdit->text();

    if (!changed)
        return false;

    m_store.save(
        isChecked(),
        newTransport,
        m_commandLineEdit->text(),
        m_argsWidget->items(),
        m_workingDirLineEdit->text(),
        m_urlLineEdit->text(),
        m_bearerTokenLineEdit->text()
    );

    return true;
}

void McpServerSettingsWidget::updateTransportPanelVisibility()
{
    // Page 0 = Stdio, Page 1 = HTTP
    m_transportStack->setCurrentIndex(m_transportCombo->currentIndex());
}

void McpServerSettingsWidget::setupUi()
{
    setTitle(m_store.serverName());
    setCheckable(true);
    setToolTip(tr("Enable or disable the '%1' MCP server").arg(m_store.serverName()));

    // Transport combo
    m_transportCombo->addItem(tr("Std I/O"), static_cast<int>(McpServerConfig::Stdio));
    m_transportCombo->addItem(tr("HTTP"),  static_cast<int>(McpServerConfig::Http));

    connect(m_transportCombo.get(), &QComboBox::currentIndexChanged,
            this, &McpServerSettingsWidget::updateTransportPanelVisibility);

    // -- Stdio panel --
    m_commandLineEdit->setPlaceholderText(tr("Executable path or command"));
    m_workingDirLineEdit->setPlaceholderText(tr("Working directory (optional)"));

    auto *stdioPanel = new QWidget(this);
    {
        using namespace Layouting;
        Form{
            tr("Command:"),     m_commandLineEdit.get(),    br,
            tr("Working dir:"), m_workingDirLineEdit.get(), br,
            tr("Arguments:"),   m_argsWidget->toolBar(),    br,
            Span(2,             m_argsWidget.get()),
            spacing(2), customMargins(4, 4, 4, 4),
        }.attachTo(stdioPanel);
    }

    // Constrain args list height — it doesn't need to show many rows
    m_argsWidget->setMaximumHeight(80);

    // -- HTTP panel --
    m_urlLineEdit->setPlaceholderText(tr("http://host:port/mcp"));
    m_bearerTokenLineEdit->setPlaceholderText(tr("Bearer token (optional)"));
    m_bearerTokenLineEdit->setEchoMode(QLineEdit::Password);

    auto *httpPanel = new QWidget(this);
    {
        using namespace Layouting;
        Form{
            tr("URL:"),          m_urlLineEdit.get(),          br,
            tr("Bearer token:"), m_bearerTokenLineEdit.get(),
            spacing(2), customMargins(4, 4, 4, 4),
        }.attachTo(httpPanel);
    }

    m_transportStack->addWidget(stdioPanel);
    m_transportStack->addWidget(httpPanel);

    // -- Remove button --
    auto *removeButton = new QPushButton(tr("Remove server"), this);
    removeButton->setToolTip(tr("Delete this server from the configuration"));
    connect(removeButton, &QPushButton::clicked, this, [this] {
        emit removeRequested(m_store.serverName());
    });

    // -- Top-level layout --
    using namespace Layouting;
    Form{
         tr("Transport:"), Row{ m_transportCombo.get(), Stretch(1) }, br,
         Span(2, m_transportStack.get()),                             br,
         Span(2, Row{ Stretch(1), removeButton }),
         spacing(4), customMargins(8, 8, 8, 8),
         }.attachTo(this);
}

} // namespace QmlDesigner
