// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiprovidersettingswidget.h"

#include "aiproviderdata.h"
#include "stringlistwidget.h"

#include <componentcore/theme.h>
#include <utils/layoutbuilder.h>

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>

namespace QmlDesigner {

AiProviderSettingsWidget::AiProviderSettingsWidget(const QString &providerName, QWidget *parent)
    : QGroupBox(parent)
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
    const auto providerData = AiProviderData::defaultProviders().value(m_config.providerName());

    QUrl url = m_config.url();
    if (url.isEmpty())
        url = providerData.url;
    m_url->setText(url.toString());

    m_models->setItems(m_config.modelIds(), providerData.models);
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

    const int iconSize = 24;
    QPushButton *resetUrlButton = new QPushButton(this);
    resetUrlButton->setIcon(Theme::iconFromName(Theme::resetView_small).pixmap(iconSize));
    resetUrlButton->setToolTip(tr("Reset Url"));
    resetUrlButton->setFixedSize({iconSize, iconSize});
    connect(resetUrlButton, &QAbstractButton::clicked, this, [this] {
        const auto providerData = AiProviderData::defaultProviders().value(m_config.providerName());
        m_url->setText(providerData.url.toString());
    });

    using namespace Layouting;
    Column{
        Column{
            createLabel(tr("Url:")),
            Row{
                m_url.get(),
                resetUrlButton,
            },
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
