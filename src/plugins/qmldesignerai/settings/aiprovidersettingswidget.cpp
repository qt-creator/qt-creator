// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiprovidersettingswidget.h"

#include "aiproviderdata.h"
#include "stringlistwidget.h"

#include <componentcore/theme.h>
#include <utils/layoutbuilder.h>

#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolBar>

namespace QmlDesigner {

static constexpr QSize iconSize = {24, 24};

static QIcon toolButtonIcon(Theme::Icon type)
{
    const QIcon normalIcon = Theme::iconFromName(type);
    const QIcon disabledIcon
        = Theme::iconFromName(type, Theme::getColor(Theme::Color::DStextSelectedTextColor));
    QIcon icon;
    icon.addPixmap(normalIcon.pixmap(iconSize), QIcon::Normal, QIcon::On);
    icon.addPixmap(disabledIcon.pixmap(iconSize), QIcon::Disabled, QIcon::On);
    return icon;
}

AiProviderSettingsWidget::AiProviderSettingsWidget(const QString &providerName, QWidget *parent)
    : QGroupBox(parent)
    , m_config(providerName)
    , m_urlLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_apiKeyLineEdit(Utils::makeUniqueObjectPtr<QLineEdit>())
    , m_modelsListWidget(Utils::makeUniqueObjectPtr<StringListWidget>())
{
    setupUi();
    load();
}

void AiProviderSettingsWidget::load()
{
    setChecked(m_config.isChecked());
    m_apiKeyLineEdit->setText(m_config.apiKey());

    const AiProviderData providerData = AiProviderData::defaultProviderData(m_config.providerName());

    QUrl url = m_config.url();
    if (url.isEmpty())
        url = providerData.url;
    m_urlLineEdit->setText(url.toString());

    m_modelsListWidget->setItems(m_config.modelIds(), providerData.models);
}

bool AiProviderSettingsWidget::save()
{
    bool changed = m_config.isChecked() != isChecked()
                || m_config.apiKey() != m_apiKeyLineEdit->text()
                || m_config.url() != m_urlLineEdit->text()
                || m_config.modelIds() != m_modelsListWidget->items();
    if (!changed)
        return false;

    m_config.save(isChecked(), m_urlLineEdit->text(), m_apiKeyLineEdit->text(), m_modelsListWidget->items());
    return true;
}

const AiProviderConfig AiProviderSettingsWidget::config() const
{
    return m_config;
}

bool AiProviderSettingsWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ToolTip: {
        if (!m_mouseOverCheckBox)
            event->ignore();
        else
            return QGroupBox::event(event);
    } break;
    default:
        return QGroupBox::event(event);
    }
    return true;
}

void AiProviderSettingsWidget::mouseMoveEvent(QMouseEvent *event)
{
    QStyleOptionGroupBox box;
    initStyleOption(&box);
    QStyle::SubControl mouseOverTest
        = style()
              ->hitTestComplexControl(QStyle::CC_GroupBox, &box, event->position().toPoint(), this);

    m_mouseOverCheckBox
        = (mouseOverTest == QStyle::SC_GroupBoxCheckBox
           || mouseOverTest == QStyle::SC_GroupBoxLabel);

    return QGroupBox::mouseMoveEvent(event);
}

void AiProviderSettingsWidget::setupUi()
{
    setTitle(m_config.providerName());
    setCheckable(true);
    setMouseTracking(true);
    setToolTip(tr("Show or hide %1 models in the AI Assistant model selector drop down")
                   .arg(m_config.providerName()));

    using namespace Qt::StringLiterals;
    auto createLabel = [this](const QString &txt) -> QLabel * {
        QLabel *label = new QLabel(txt, this);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        label->setTextFormat(Qt::RichText);
        label->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
        return label;
    };

    m_apiKeyLineEdit->setPlaceholderText(tr("Enter %1 API key to use its models").arg(m_config.providerName()));

    QPushButton *resetUrlButton = new QPushButton(this);
    resetUrlButton->setIcon(toolButtonIcon(Theme::resetView_small));
    resetUrlButton->setToolTip(tr("Reset Url"));
    resetUrlButton->setFixedSize(iconSize);
    connect(resetUrlButton, &QAbstractButton::clicked, this, [this] {
        const AiProviderData providerData = AiProviderData::defaultProviderData(m_config.providerName());
        m_urlLineEdit->setText(providerData.url.toString());
    });

    using namespace Layouting;
    Column{
        Column{
            createLabel(tr("Url:")),
            Row{
                m_urlLineEdit.get(),
                resetUrlButton,
            },
            spacing(3),
        },
        Column{
            createLabel(tr("API key:")),
            m_apiKeyLineEdit.get(),
            spacing(3),
        },
        Column{
            Row{
                createLabel(tr("Models:")),
                m_modelsListWidget->toolBar(),
            },
            m_modelsListWidget.get(),
            spacing(3),
        },
        spacing(10),
    }.attachTo(this);
}

} // namespace QmlDesigner
