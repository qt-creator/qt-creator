/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "targetsettingswidget.h"
#include "ui_targetsettingswidget.h"

#include <QPushButton>

using namespace ProjectExplorer::Internal;

TargetSettingsWidget::TargetSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TargetSettingsWidget),
    m_targetSelector(new TargetSelector(this))
{
    ui->setupUi(this);
    ui->header->setStyleSheet(QLatin1String("QWidget#header {"
        "border-image: url(:/projectexplorer/images/targetseparatorbackground.png) 43 0 0 0 repeat;"
        "}"));

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(5, 3, 0, 0);
    ui->header->setLayout(headerLayout);

    QWidget *buttonWidget = new QWidget(ui->header);
    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(4);
    buttonWidget->setLayout(buttonLayout);
    m_addButton = new QPushButton(tr("Add Kit"), buttonWidget);
    buttonLayout->addWidget(m_addButton);
    m_manageButton = new QPushButton(tr("Manage Kits..."), buttonWidget);
    connect(m_manageButton, SIGNAL(clicked()), this, SIGNAL(manageButtonClicked()));
    buttonLayout->addWidget(m_manageButton);
    headerLayout->addWidget(buttonWidget, 0, Qt::AlignVCenter);

    headerLayout->addWidget(m_targetSelector, 0, Qt::AlignBottom);
    headerLayout->addStretch(10);
    connect(m_targetSelector, SIGNAL(currentChanged(int,int)),
            this, SIGNAL(currentChanged(int,int)));
    connect(m_targetSelector, SIGNAL(toolTipRequested(QPoint,int)),
            this, SIGNAL(toolTipRequested(QPoint,int)));
    connect(m_targetSelector, SIGNAL(menuShown(int)),
            this, SIGNAL(menuShown(int)));

    QPalette shadowPal = palette();
    QLinearGradient grad(0, 0, 0, 2);
    grad.setColorAt(0, QColor(0, 0, 0, 60));
    grad.setColorAt(1, Qt::transparent);
    shadowPal.setBrush(QPalette::All, QPalette::Window, grad);
    ui->shadow->setPalette(shadowPal);
    ui->shadow->setAutoFillBackground(true);
}

TargetSettingsWidget::~TargetSettingsWidget()
{
    delete ui;
}

void TargetSettingsWidget::insertTarget(int index, const QString &name)
{
    m_targetSelector->insertTarget(index, name);
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
    ui->scrollArea->setWidget(widget);
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

void TargetSettingsWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
