// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpserversettingswidget.h"

#include "aidefaultsettings.h"
#include "stringlistwidget.h"

#include <aiassistantconstants.h>

#include <componentcore/theme.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QToolBar>

namespace QmlDesigner {

static QIcon toolButtonIcon(Theme::Icon type)
{
    const QIcon normalIcon = Theme::iconFromName(type, Theme::getColor(Theme::Color::DS_text_default));
    const QIcon disabledIcon = Theme::iconFromName(type, Theme::getColor(Theme::Color::DS_text_muted));
    QIcon icon;
    icon.addPixmap(normalIcon.pixmap(14), QIcon::Normal, QIcon::On);
    icon.addPixmap(disabledIcon.pixmap(14), QIcon::Disabled, QIcon::On);
    return icon;
}

McpServerSettingsWidget::McpServerSettingsWidget(const QString &serverName, QWidget *parent)
    : QGroupBox(parent)
    , m_store(serverName)
    , m_enabledCheckBox(Utils::makeUniqueObjectPtr<QCheckBox>(tr("Enabled")))
    , m_transportCombo(Utils::makeUniqueObjectPtr<QComboBox>())
    , m_commandLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_workingDirLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_argsWidget(Utils::makeUniqueObjectPtr<StringListWidget>(
          serverName != Constants::qmlServerName))
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
    m_enabledCheckBox->setChecked(m_store.isEnabled());

    const int transportIndex = (m_store.transport() == McpServerConfig::Http) ? 1 : 0;
    m_transportCombo->setCurrentIndex(transportIndex);
    updateTransportPanelVisibility();

    // Stdio
    m_commandLineEdit->setText(m_store.command());
    m_commandLineEdit->setMaxLength(512);
    m_workingDirLineEdit->setText(m_store.workingDir());
    m_workingDirLineEdit->setMaxLength(512);
    QStringList defaultArgs = m_store.serverName() == Constants::qmlServerName
                            ? AiDefaultSettings::mcpServerData(Constants::qmlServerName).args
                            : QStringList();
    m_argsWidget->setItems(m_store.args(), defaultArgs);

    // HTTP
    m_urlLineEdit->setText(m_store.url());
    m_urlLineEdit->setMaxLength(2048);
    m_bearerTokenLineEdit->setText(m_store.bearerToken());
    m_bearerTokenLineEdit->setMaxLength(1024);
    m_bearerTokenLineEdit->setEchoMode(QLineEdit::Password);
}

bool McpServerSettingsWidget::save()
{
    const McpServerConfig::Transport newTransport
        = m_transportCombo->currentIndex() == 1 ? McpServerConfig::Http : McpServerConfig::Stdio;

    const bool changed = m_store.isEnabled() != m_enabledCheckBox->isChecked()
                      || m_store.transport() != newTransport
                      || m_store.command() != m_commandLineEdit->text()
                      || m_store.workingDir() != m_workingDirLineEdit->text()
                      || m_store.args() != m_argsWidget->items()
                      || m_store.url() != m_urlLineEdit->text()
                      || m_store.bearerToken() != m_bearerTokenLineEdit->text();

    if (!changed)
        return false;

    m_store.save(
        m_enabledCheckBox->isChecked(),
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
    using namespace Layouting;

    setTitle(m_store.serverName());

    m_enabledCheckBox->setToolTip(tr("Enable or disable the '%1' MCP server").arg(m_store.serverName()));

    // Transport combo
    m_transportCombo->addItem(tr("Std I/O"), static_cast<int>(McpServerConfig::Stdio));
    m_transportCombo->addItem(tr("HTTP"),  static_cast<int>(McpServerConfig::Http));

    connect(m_transportCombo.get(), &QComboBox::currentIndexChanged,
            this, &McpServerSettingsWidget::updateTransportPanelVisibility);

    // -- Stdio panel --
    m_commandLineEdit->setPlaceholderText(tr("Executable path or command"));
    m_workingDirLineEdit->setPlaceholderText(tr("Working directory (optional)"));

    if (m_store.serverName() == Constants::qmlServerName) {
        QAction *resetCommandAction = m_commandLineEdit->addAction(toolButtonIcon(Theme::resetView_small),
                                                           QLineEdit::TrailingPosition);
        resetCommandAction->setToolTip(tr("Reset QML server command to default."));
        connect(resetCommandAction, &QAction::triggered, this, [this] {
            const McpServerData def = AiDefaultSettings::mcpServerData(Constants::qmlServerName);
            m_commandLineEdit->setText(def.command);
        });

        QAction *resetWorkingDirdAction = m_workingDirLineEdit->addAction(toolButtonIcon(Theme::resetView_small),
                                                                   QLineEdit::TrailingPosition);
        resetWorkingDirdAction->setToolTip(tr("Reset Qml server working directory to default."));
        connect(resetWorkingDirdAction, &QAction::triggered, this, [this] {
            const McpServerData def = AiDefaultSettings::mcpServerData(Constants::qmlServerName);
            m_workingDirLineEdit->setText(def.workingDir);
        });
    }

    auto *stdioPanel = new QWidget(this);
    Form{
        tr("Command:"),     m_commandLineEdit.get(),    br,
        tr("Working dir:"), m_workingDirLineEdit.get(), br,
        tr("Arguments:"),   m_argsWidget->toolBar(),    br,
        Span(2,             m_argsWidget.get()),
        spacing(2), customMargins(4, 4, 4, 4),
    }.attachTo(stdioPanel);

    // Constrain args list height — it doesn't need to show many rows
    m_argsWidget->setMaximumHeight(80);

    // -- HTTP panel --
    m_urlLineEdit->setPlaceholderText(tr("http://host:port/mcp"));
    m_urlLineEdit->setMaxLength(2048);
    m_bearerTokenLineEdit->setPlaceholderText(tr("Bearer token (optional)"));
    m_bearerTokenLineEdit->setMaxLength(1024);
    m_bearerTokenLineEdit->setEchoMode(QLineEdit::Password);

    QAction *toggleBearerAction = m_bearerTokenLineEdit->addAction(toolButtonIcon(Theme::visible_medium),
                                                                   QLineEdit::TrailingPosition);
    toggleBearerAction->setToolTip(tr("Show/Hide API Key"));
    toggleBearerAction->setCheckable(true);
    connect(toggleBearerAction, &QAction::toggled, this, [&](bool checked) {
        m_bearerTokenLineEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    auto *httpPanel = new QWidget(this);
    Form{
        tr("URL:"),          m_urlLineEdit.get(),          br,
        tr("Bearer token:"), m_bearerTokenLineEdit.get(),
        spacing(2), customMargins(4, 4, 4, 4),
    }.attachTo(httpPanel);

    m_transportStack->addWidget(stdioPanel);
    m_transportStack->addWidget(httpPanel);

    // -- Remove button --
    // qml_server is not deletable
    QPushButton *removeButton = nullptr;
    if (m_store.serverName() != Constants::qmlServerName) {
        removeButton = new QPushButton(tr("Remove server"), this);
        removeButton->setToolTip(tr("Delete this server from the configuration"));
        connect(removeButton, &QPushButton::clicked, this, [this] {
            emit removeRequested(m_store.serverName());
        });
    }

    // -- Top-level layout --
    Form{
        m_enabledCheckBox.get(),                                     br,
        tr("Transport:"), Row{ m_transportCombo.get(), Stretch(1) }, br,
        Span(2, m_transportStack.get()),                             br,
        Span(2, removeButton ? Row{ Stretch(1), removeButton }
                             : Row{ Stretch(1) }),
        spacing(4), customMargins(8, 8, 8, 8),
    }.attachTo(this);
}

} // namespace QmlDesigner
