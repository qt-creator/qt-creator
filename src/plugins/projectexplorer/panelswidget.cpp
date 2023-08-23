// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "panelswidget.h"

#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace {

const int ABOVE_HEADING_MARGIN = 10;
const int CONTENTS_MARGIN = 5;
const int BELOW_CONTENTS_MARGIN = 16;

}

///
// PanelsWidget
///

PanelsWidget::PanelsWidget(QWidget *parent, bool addStretch) : QWidget(parent)
{
    m_root = new QWidget(nullptr);
    m_root->setFocusPolicy(Qt::NoFocus);
    m_root->setContentsMargins(0, 0, 0, 0);

    const auto scroller = new QScrollArea(this);
    scroller->setWidget(m_root);
    scroller->setFrameStyle(QFrame::NoFrame);
    scroller->setWidgetResizable(true);
    scroller->setFocusPolicy(Qt::NoFocus);

    // The layout holding the individual panels:
    auto topLayout = new QVBoxLayout(m_root);
    topLayout->setContentsMargins(PanelVMargin, 0, PanelVMargin, 0);
    topLayout->setSpacing(0);

    m_layout = new QVBoxLayout;
    m_layout->setSpacing(0);

    topLayout->addLayout(m_layout);
    if (addStretch)
        topLayout->addStretch(1);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(new Utils::StyledBar(this));
    layout->addWidget(scroller);

    //layout->addWidget(new FindToolBarPlaceHolder(this));
}

PanelsWidget::PanelsWidget(const QString &displayName, QWidget *widget)
    : PanelsWidget(nullptr)
{
    addPropertiesPanel(displayName);
    addWidget(widget);
}

PanelsWidget::PanelsWidget(const QString &displayName, ProjectSettingsWidget *widget)
    : PanelsWidget(nullptr, !widget->expanding())
{
    addPropertiesPanel(displayName);
    addGlobalSettingsProperties(widget);
    addWidget(widget);
}

PanelsWidget::~PanelsWidget() = default;

/*
 * Add a widget with heading information into the layout of the PanelsWidget.
 *
 *     ...
 * +------------+ ABOVE_HEADING_MARGIN
 * | name       |
 * +------------+
 * | line       |
 * +------------+ ABOVE_CONTENTS_MARGIN
 * | widget     |
 * +------------+ BELOW_CONTENTS_MARGIN
 */
void PanelsWidget::addPropertiesPanel(const QString &displayName)
{
    // name:
    auto nameLabel = new QLabel(m_root);
    nameLabel->setText(displayName);
    nameLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
    QFont f = nameLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.6);
    nameLabel->setFont(f);
    m_layout->addWidget(nameLabel);
    m_layout->addWidget(Layouting::createHr());
}

void PanelsWidget::addWidget(QWidget *widget)
{
    widget->setContentsMargins(0, CONTENTS_MARGIN, 0, BELOW_CONTENTS_MARGIN);
    widget->setParent(m_root);
    m_layout->addWidget(widget);
}

void PanelsWidget::addGlobalSettingsProperties(ProjectSettingsWidget *widget)
{
    if (!widget->isUseGlobalSettingsCheckBoxVisible() && !widget->isUseGlobalSettingsLabelVisible())
        return;
    m_layout->setContentsMargins(0, 0, 0, 0);
    const auto useGlobalSettingsCheckBox = new QCheckBox;
    useGlobalSettingsCheckBox->setChecked(widget->useGlobalSettings());
    useGlobalSettingsCheckBox->setEnabled(widget->isUseGlobalSettingsCheckBoxEnabled());

    const QString labelText = widget->isUseGlobalSettingsCheckBoxVisible()
                                  ? QStringLiteral("Use <a href=\"dummy\">global settings</a>")
                                  : QStringLiteral("<a href=\"dummy\">Global settings</a>");
    const auto settingsLabel = new QLabel(labelText);
    settingsLabel->setEnabled(widget->isUseGlobalSettingsCheckBoxEnabled());

    const auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->setContentsMargins(0, CONTENTS_MARGIN, 0, CONTENTS_MARGIN);
    horizontalLayout->setSpacing(CONTENTS_MARGIN);

    if (widget->isUseGlobalSettingsCheckBoxVisible()) {
        horizontalLayout->addWidget(useGlobalSettingsCheckBox);

        connect(widget, &ProjectSettingsWidget::useGlobalSettingsCheckBoxEnabledChanged,
                this, [useGlobalSettingsCheckBox, settingsLabel](bool enabled) {
                    useGlobalSettingsCheckBox->setEnabled(enabled);
                    settingsLabel->setEnabled(enabled);
                });
        connect(useGlobalSettingsCheckBox, &QCheckBox::stateChanged,
                widget, &ProjectSettingsWidget::setUseGlobalSettings);
        connect(widget, &ProjectSettingsWidget::useGlobalSettingsChanged,
                useGlobalSettingsCheckBox, &QCheckBox::setChecked);
    }

    if (widget->isUseGlobalSettingsLabelVisible()) {
        horizontalLayout->addWidget(settingsLabel);
        connect(settingsLabel, &QLabel::linkActivated, this, [widget] {
            Core::ICore::showOptionsDialog(widget->globalSettingsId());
        });
    }
    horizontalLayout->addStretch(1);
    m_layout->addLayout(horizontalLayout);
    m_layout->addWidget(Layouting::createHr());
}

} // ProjectExplorer
