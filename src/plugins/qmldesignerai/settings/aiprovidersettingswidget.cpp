// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiprovidersettingswidget.h"

#include "stringlistwidget.h"

#include <utils/layoutbuilder.h>

#include <QLabel>
#include <QLineEdit>
#include <QToolBar>

namespace QmlDesigner {

AiProviderSettingsWidget::AiProviderSettingsWidget(const QString &providerName, QWidget *parent)
    : QGroupBox(parent)
    , m_provider(AiProviderData::findProvider(providerName))
    , m_config(providerName)
    , m_url(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_apiKey(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_models(Utils::makeUniqueObjectPtr<StringListWidget>())
{
    setupUi();
    load();
}

void AiProviderSettingsWidget::load()
{
    m_apiKey->setText(m_config.apiKey());

    QUrl url = m_config.url();
    if (url.isEmpty())
        url = m_provider.defaultUrl;
    m_url->setText(url.toString());

    QStringList modelIds = m_config.modelIds();
    if (modelIds.isEmpty())
        modelIds = m_provider.models;
    m_models->setItems(modelIds);
}

bool AiProviderSettingsWidget::save()
{
    bool changed = m_config.apiKey() != m_apiKey->text() || m_config.url() != m_url->text()
                   || m_config.modelIds() != m_models->items();
    if (!changed)
        return false;

    m_config.save(m_url->text(), m_apiKey->text(), m_models->items());
    return true;
}

const AiProviderConfig AiProviderSettingsWidget::config() const
{
    return m_config;
}

void AiProviderSettingsWidget::setupUi()
{
    setTitle(m_config.providerName());

    using namespace Qt::StringLiterals;
    auto createLabel = [this](const QString &txt) -> QLabel * {
        QLabel *label = new QLabel(txt, this);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        label->setTextFormat(Qt::RichText);
        label->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
        return label;
    };

    using namespace Layouting;
    Column{
        Column{
            createLabel(tr("Url:")),
            m_url.get(),
            spacing(3),
        },
        Column{
            createLabel(tr("API key:")),
            m_apiKey.get(),
            spacing(3),
        },
        Column{
            Row{
                createLabel(tr("Models:")),
                m_models->toolBar(),
            },
            m_models.get(),
            spacing(3),
        },
        spacing(10),
    }.attachTo(this);
}

} // namespace QmlDesigner
