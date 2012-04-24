/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "targetsettingswidget.h"
#include "ui_targetsettingswidget.h"

static int WIDTH = 900;

using namespace ProjectExplorer::Internal;

TargetSettingsWidget::TargetSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TargetSettingsWidget),
    m_targetSelector(new TargetSelector(this))
{
    ui->setupUi(this);
    ui->separator->setStyleSheet(QLatin1String("* { "
        "background-image: url(:/projectexplorer/images/targetseparatorbackground.png);"
        "}"));
    m_targetSelector->raise();
    connect(m_targetSelector, SIGNAL(removeButtonClicked()),
            this, SIGNAL(removeButtonClicked()));
    connect(m_targetSelector, SIGNAL(currentChanged(int,int)),
            this, SIGNAL(currentChanged(int,int)));

    m_shadow = new QWidget(this);

    // Create shadow below targetselector
    m_targetSelector->raise();
    QPalette shadowPal = palette();
    QLinearGradient grad(0, 0, 0, 2);
    grad.setColorAt(0, QColor(0, 0, 0, 60));
    grad.setColorAt(1, Qt::transparent);
    shadowPal.setBrush(QPalette::All, QPalette::Window, grad);
    m_shadow->setPalette(shadowPal);
    m_shadow->setAutoFillBackground(true);

    updateTargetSelector();
}

TargetSettingsWidget::~TargetSettingsWidget()
{
    delete ui;
}

void TargetSettingsWidget::insertTarget(int index, const QString &name)
{
    m_targetSelector->insertTarget(index, name);
    updateTargetSelector();
}

void TargetSettingsWidget::renameTarget(int index, const QString &name)
{
    m_targetSelector->renameTarget(index, name);
    // geometry won't change, so no need to updateTargetSelector()
}

void TargetSettingsWidget::removeTarget(int index)
{
    m_targetSelector->removeTarget(index);
    updateTargetSelector();
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
    m_targetSelector->setAddButtonEnabled(enabled);
}

void TargetSettingsWidget::setAddButtonMenu(QMenu *menu)
{
    m_targetSelector->setAddButtonMenu(menu);
}

void TargetSettingsWidget::setRemoveButtonEnabled(bool enabled)
{
    m_targetSelector->setRemoveButtonEnabled(enabled);
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

bool TargetSettingsWidget::isAddButtonEnabled() const
{
    return m_targetSelector->isAddButtonEnabled();
}

bool TargetSettingsWidget::isRemoveButtonEnabled() const
{
    return m_targetSelector->isRemoveButtonEnabled();
}

void TargetSettingsWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_shadow->setGeometry(0, m_targetSelector->height() + 3, width(), 2);
}

void TargetSettingsWidget::updateTargetSelector()
{
    m_targetSelector->setGeometry((WIDTH-m_targetSelector->minimumSizeHint().width())/2, 13,
        m_targetSelector->minimumSizeHint().width(),
        m_targetSelector->minimumSizeHint().height());
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
