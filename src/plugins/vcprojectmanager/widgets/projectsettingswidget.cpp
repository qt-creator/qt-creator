/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "projectsettingswidget.h"
#include "ui_projectsettingswidget.h"

#include <QSplitter>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>

namespace VcProjectManager {
namespace Internal {

ProjectSettingsWidget::ProjectSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProjectSettingsWidget)
{
    ui->setupUi(this);
    m_widgetList = new QListWidget;
    m_widgetList->setMinimumWidth(150);
    m_widgetList->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_widgetList->setMaximumWidth(300);
    m_stackedWidgets = new QStackedWidget;

    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_widgetList);
    splitter->addWidget(m_stackedWidgets);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(splitter);

    ui->m_mainWidget->setLayout(layout);
    connect(m_widgetList, SIGNAL(currentRowChanged(int)), m_stackedWidgets, SLOT(setCurrentIndex(int)));
    connect(ui->m_buttonBox, SIGNAL(accepted()), this, SIGNAL(okButtonClicked()));
    connect(ui->m_buttonBox, SIGNAL(rejected()), this, SIGNAL(cancelButtonClicked()));
}

ProjectSettingsWidget::~ProjectSettingsWidget()
{
    delete ui;
}

void ProjectSettingsWidget::addWidget(const QString &name, QWidget *widget)
{
    m_widgetList->addItem(name);
    m_stackedWidgets->addWidget(widget);
}

void ProjectSettingsWidget::addWidget(const QString &name, const QIcon &icon, QWidget *widget)
{
    QListWidgetItem *item = new QListWidgetItem(icon, name);
    m_widgetList->addItem(item);
    m_stackedWidgets->addWidget(widget);
}

QWidget *ProjectSettingsWidget::widget(const QString &name)
{
    for (int i = 0; i < m_widgetList->count(); ++i) {
        QListWidgetItem *item = m_widgetList->item(i);
        if (item && item->text() == name)
            return m_stackedWidgets->widget(i);
    }

    return 0;
}

} // namespace Internal
} // namespace VcProjectManager
