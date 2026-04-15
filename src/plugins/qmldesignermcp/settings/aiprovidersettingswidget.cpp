// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiprovidersettingswidget.h"

#include "aidefaultsettings.h"
#include "stringlistwidget.h"

#include <componentcore/theme.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>
#include <QToolButton>

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

AiProviderSettingsWidget::AiProviderSettingsWidget(const QString &providerName, QWidget *parent)
    : QGroupBox(parent)
    , m_config(providerName)
    , m_enabledCheckBox(Utils::makeUniqueObjectPtr<QCheckBox>(tr("Enabled")))
    , m_urlLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_apiKeyLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_modelsListWidget(Utils::makeUniqueObjectPtr<StringListWidget>())
{
    setupUi();
    load();
}

void AiProviderSettingsWidget::load()
{
    m_enabledCheckBox->setChecked(m_config.isChecked());
    m_apiKeyLineEdit->setText(m_config.apiKey());

    const AiProviderData providerData = AiDefaultSettings::providerData(m_config.providerName());

    m_urlLineEdit->setText(m_config.url().isEmpty() ? providerData.url : m_config.url());

    m_modelsListWidget->setItems(m_config.modelIds(), providerData.models);
}

bool AiProviderSettingsWidget::save()
{
    bool changed = m_config.isChecked() != m_enabledCheckBox->isChecked()
                || m_config.apiKey() != m_apiKeyLineEdit->text()
                || m_config.url() != m_urlLineEdit->text()
                || m_config.modelIds() != m_modelsListWidget->items();
    if (!changed)
        return false;

    m_config.save(m_enabledCheckBox->isChecked(), m_urlLineEdit->text(), m_apiKeyLineEdit->text(),
                  m_modelsListWidget->items());
    return true;
}

const AiProviderConfig AiProviderSettingsWidget::config() const
{
    return m_config;
}

void AiProviderSettingsWidget::setupUi()
{
    setTitle(m_config.providerName());

    m_enabledCheckBox->setToolTip(tr("Show or hide %1 models in the AI Assistant model selector drop down")
                   .arg(m_config.providerName()));

    using namespace Qt::StringLiterals;

    m_urlLineEdit->setMaxLength(2048);

    m_apiKeyLineEdit->setPlaceholderText(tr("Enter %1 API key to use its models").arg(m_config.providerName()));
    m_apiKeyLineEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyLineEdit->setMaxLength(1024);

    QAction *resetUrlAction = m_urlLineEdit->addAction(toolButtonIcon(Theme::resetView_small),
                                                       QLineEdit::TrailingPosition);
    resetUrlAction->setToolTip(tr("Reset Url"));
    connect(resetUrlAction, &QAction::triggered, this, [this] {
        const AiProviderData providerData = AiDefaultSettings::providerData(m_config.providerName());
        m_urlLineEdit->setText(providerData.url);
    });

    using namespace Layouting;
    Form{
        m_enabledCheckBox.get(),                       br,
        tr("Url:"),     m_urlLineEdit.get(),           br,
        tr("API key:"), m_apiKeyLineEdit.get(),        br,
        tr("Models:"),  m_modelsListWidget->toolBar(), br,
        Span(2, m_modelsListWidget.get()),

        spacing(4),
        customMargins(8, 8, 8, 8),
    }.attachTo(this);
}

} // namespace QmlDesigner
