/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "panelswidget.h"

#include <coreplugin/icore.h>
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

PanelsWidget::PanelsWidget(QWidget *parent) : QWidget(parent)
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
    topLayout->addStretch(100);

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
    : PanelsWidget(nullptr)
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

    // line:
    auto line = new QFrame(m_root);
    line->setFrameShape(QFrame::HLine);
    line->setForegroundRole(QPalette::Midlight);
    line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_layout->addWidget(line);
}

void PanelsWidget::addWidget(QWidget *widget)
{
    widget->setContentsMargins(0, CONTENTS_MARGIN, 0, BELOW_CONTENTS_MARGIN);
    widget->setParent(m_root);
    m_layout->addWidget(widget);
}

void PanelsWidget::addGlobalSettingsProperties(ProjectSettingsWidget *widget)
{
    if (!widget->isUseGlobalSettingsCheckBoxVisible())
        return;
    m_layout->setContentsMargins(0, 0, 0, 0);
    const auto useGlobalSettingsCheckBox = new QCheckBox;
    useGlobalSettingsCheckBox->setChecked(widget->useGlobalSettings());
    useGlobalSettingsCheckBox->setEnabled(widget->isUseGlobalSettingsCheckBoxEnabled());
    const auto settingsLabel = new QLabel("Use <a href=\"dummy\">global settings</a>");
    settingsLabel->setContentsMargins(CONTENTS_MARGIN, 0, 0, 0);
    settingsLabel->setEnabled(widget->isUseGlobalSettingsCheckBoxEnabled());
    const auto horizontLayout = new QHBoxLayout;
    horizontLayout->setContentsMargins(0, CONTENTS_MARGIN, 0, CONTENTS_MARGIN);
    horizontLayout->addWidget(useGlobalSettingsCheckBox);
    horizontLayout->addWidget(settingsLabel);
    horizontLayout->addStretch(1);
    m_layout->addLayout(horizontLayout);

    auto separator = new QFrame(m_root);
    separator->setFrameShape(QFrame::HLine);
    m_layout->addWidget(separator);

    connect(widget, &ProjectSettingsWidget::useGlobalSettingsCheckBoxEnabledChanged, this,
            [useGlobalSettingsCheckBox, settingsLabel] (bool enabled) {
                useGlobalSettingsCheckBox->setEnabled(enabled);
                settingsLabel->setEnabled(enabled);
            });

    connect(useGlobalSettingsCheckBox, &QCheckBox::stateChanged,
            widget, &ProjectSettingsWidget::setUseGlobalSettings);
    connect(widget, &ProjectSettingsWidget::useGlobalSettingsChanged,
            useGlobalSettingsCheckBox, &QCheckBox::setChecked);

    connect(settingsLabel, &QLabel::linkActivated, this, [widget] {
        Core::ICore::showOptionsDialog(widget->globalSettingsId());
    });
}

} // ProjectExplorer
