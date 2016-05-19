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

#include "targetsettingswidget.h"

#include <utils/theme/theme.h>

#include <QPushButton>
#include <QVBoxLayout>

using namespace ProjectExplorer::Internal;

TargetSettingsWidget::TargetSettingsWidget(QWidget *parent) : QWidget(parent),
    m_targetSelector(new TargetSelector(this))
{
    QBrush brush(Qt::white);
    QBrush brush1(QColor(115, 115, 115, 255));
    QPalette separatorPalette;
    separatorPalette.setBrush(QPalette::Active, QPalette::Base, brush);
    separatorPalette.setBrush(QPalette::Active, QPalette::Window, brush1);
    separatorPalette.setBrush(QPalette::Inactive, QPalette::Base, brush);
    separatorPalette.setBrush(QPalette::Inactive, QPalette::Window, brush1);
    separatorPalette.setBrush(QPalette::Disabled, QPalette::Base, brush1);
    separatorPalette.setBrush(QPalette::Disabled, QPalette::Window, brush1);

    QPalette shadowPal;
    QLinearGradient grad(0, 0, 0, 2);
    grad.setColorAt(0, QColor(0, 0, 0, 60));
    grad.setColorAt(1, Qt::transparent);
    shadowPal.setBrush(QPalette::All, QPalette::Window, grad);

    auto header = new QWidget(this);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(header->sizePolicy().hasHeightForWidth());
    header->setSizePolicy(sizePolicy);

    auto separator = new QWidget(this);
    separator->setMinimumSize(QSize(0, 1));
    separator->setMaximumSize(QSize(QWIDGETSIZE_MAX, 1));
    separator->setPalette(separatorPalette);
    separator->setAutoFillBackground(true);

    auto shadow = new QWidget(this);
    shadow->setMinimumSize(QSize(0, 2));
    shadow->setMaximumSize(QSize(QWIDGETSIZE_MAX, 2));
    shadow->setPalette(shadowPal);
    shadow->setAutoFillBackground(true);

    m_scrollAreaWidgetContents = new QWidget(this);
    auto scrollLayout = new QVBoxLayout(m_scrollAreaWidgetContents);
    scrollLayout->setSpacing(0);
    scrollLayout->setContentsMargins(0, 0, 0, 0);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(0);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->addWidget(header);
    verticalLayout->addWidget(separator);
    verticalLayout->addWidget(shadow);
    verticalLayout->addWidget(m_scrollAreaWidgetContents);

    if (Utils::creatorTheme()->flag(Utils::Theme::FlatProjectsMode)) {
        separator->setVisible(false);
        shadow->setVisible(false);
    } else {
        header->setStyleSheet("border-image: url(:/projectexplorer/images/targetseparatorbackground.png) 43 0 0 0 repeat;");
    }

    auto headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(5, 3, 0, 0);
    header->setLayout(headerLayout);

    auto buttonWidget = new QWidget(header);
    auto buttonLayout = new QVBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(4);
    buttonWidget->setLayout(buttonLayout);
    m_addButton = new QPushButton(tr("Add Kit"), buttonWidget);
    buttonLayout->addWidget(m_addButton);
    m_manageButton = new QPushButton(tr("Manage Kits..."), buttonWidget);
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &TargetSettingsWidget::manageButtonClicked);
    buttonLayout->addWidget(m_manageButton);
    headerLayout->addWidget(buttonWidget, 0, Qt::AlignVCenter);

    headerLayout->addWidget(m_targetSelector, 0, Qt::AlignBottom);
    headerLayout->addStretch(10);
    connect(m_targetSelector, &TargetSelector::currentChanged,
            this, &TargetSettingsWidget::currentChanged);
    connect(m_targetSelector, &TargetSelector::toolTipRequested,
            this, &TargetSettingsWidget::toolTipRequested);
    connect(m_targetSelector, &TargetSelector::menuShown,
            this, &TargetSettingsWidget::menuShown);
}

void TargetSettingsWidget::insertTarget(int index, int subIndex, const QString &name)
{
    m_targetSelector->insertTarget(index, subIndex, name);
}

void TargetSettingsWidget::renameTarget(int index, const QString &name)
{
    m_targetSelector->renameTarget(index, name);
}

void TargetSettingsWidget::removeTarget(int index)
{
    m_targetSelector->removeTarget(index);
}

void TargetSettingsWidget::setCurrentIndex(int index)
{
    m_targetSelector->setCurrentIndex(index);
}

void TargetSettingsWidget::setCurrentSubIndex(int index)
{
    m_targetSelector->setCurrentSubIndex(index);
}

void TargetSettingsWidget::setAddButtonEnabled(bool enabled)
{
    m_addButton->setEnabled(enabled);
}

void TargetSettingsWidget::setAddButtonMenu(QMenu *menu)
{
    m_addButton->setMenu(menu);
}

void TargetSettingsWidget::setTargetMenu(QMenu *menu)
{
    m_targetSelector->setTargetMenu(menu);
}

QString TargetSettingsWidget::targetNameAt(int index) const
{
    return m_targetSelector->targetAt(index).name;
}

void TargetSettingsWidget::setCentralWidget(QWidget *widget)
{
    if (m_centralWidget)
        m_scrollAreaWidgetContents->layout()->removeWidget(m_centralWidget);
    m_centralWidget = widget;
    m_scrollAreaWidgetContents->layout()->addWidget(m_centralWidget);
}

int TargetSettingsWidget::targetCount() const
{
    return m_targetSelector->targetCount();
}

int TargetSettingsWidget::currentIndex() const
{
    return m_targetSelector->currentIndex();
}

int TargetSettingsWidget::currentSubIndex() const
{
    return m_targetSelector->currentSubIndex();
}
