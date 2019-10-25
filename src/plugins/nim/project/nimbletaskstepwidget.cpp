/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimbletaskstepwidget.h"
#include "ui_nimbletaskstepwidget.h"

#include "nimbleproject.h"
#include "nimbletaskstep.h"

#include <utils/algorithm.h>

using namespace Nim;
using namespace ProjectExplorer;

NimbleTaskStepWidget::NimbleTaskStepWidget(NimbleTaskStep *bs)
    : BuildStepConfigWidget(bs)
    , ui(new Ui::NimbleTaskStepWidget)
{
    ui->setupUi(this);

    auto buildSystem = dynamic_cast<NimbleBuildSystem *>(bs->buildSystem());
    QTC_ASSERT(buildSystem, return);

    ui->taskList->setModel(&m_tasks);
    QObject::connect(&m_tasks, &QAbstractItemModel::dataChanged, this, &NimbleTaskStepWidget::onDataChanged);

    updateTaskList();
    QObject::connect(buildSystem, &NimbleBuildSystem::tasksChanged, this, &NimbleTaskStepWidget::updateTaskList);

    selectTask(bs->taskName());
    QObject::connect(bs, &NimbleTaskStep::taskNameChanged, this, &NimbleTaskStepWidget::selectTask);
    QObject::connect(bs, &NimbleTaskStep::taskNameChanged, this, &NimbleTaskStepWidget::recreateSummary);
    QObject::connect(this, &NimbleTaskStepWidget::selectedTaskChanged, bs, &NimbleTaskStep::setTaskName);

    ui->taskArgumentsLineEdit->setText(bs->taskArgs());
    QObject::connect(bs, &NimbleTaskStep::taskArgsChanged, ui->taskArgumentsLineEdit, &QLineEdit::setText);
    QObject::connect(bs, &NimbleTaskStep::taskArgsChanged, this, &NimbleTaskStepWidget::recreateSummary);
    QObject::connect(ui->taskArgumentsLineEdit, &QLineEdit::textChanged, bs ,&NimbleTaskStep::setTaskArgs);


    setSummaryUpdater([this, bs] {
        return QString("<b>%1:</b> nimble %2 %3")
                .arg(displayName())
                .arg(bs->taskName())
                .arg(bs->taskArgs());
    });
}

NimbleTaskStepWidget::~NimbleTaskStepWidget()
{
    delete ui;
}

void NimbleTaskStepWidget::updateTaskList()
{
    auto buildSystem = dynamic_cast<NimbleBuildSystem *>(step()->buildSystem());
    QTC_ASSERT(buildSystem, return);
    const std::vector<NimbleTask> &tasks = buildSystem->tasks();

    QSet<QString> newTasks;
    for (const NimbleTask &t : tasks)
        newTasks.insert(t.name);

    QSet<QString> currentTasks;
    for (int i = 0; i < m_tasks.rowCount(); ++i)
        currentTasks.insert(m_tasks.item(i)->text());

    const QSet<QString> added = newTasks - currentTasks;
    const QSet<QString> removed = currentTasks - newTasks;

    for (const QString &name : added) {
        auto item = new QStandardItem();
        item->setText(name);
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        item->setEditable(false);
        item->setSelectable(false);
        m_tasks.appendRow(item);
    }

    for (int i = m_tasks.rowCount() - 1; i >= 0; i--)
        if (removed.contains(m_tasks.item(i)->text()))
            m_tasks.removeRow(i);

    m_tasks.sort(0);
}

void NimbleTaskStepWidget::selectTask(const QString &name)
{
    if (m_selecting)
        return;
    m_selecting = true;

    QList<QStandardItem*> items = m_tasks.findItems(name);
    QStandardItem* item = items.empty() ? nullptr : items.front();
    uncheckedAllDifferentFrom(item);
    if (item)
        item->setCheckState(Qt::Checked);

    emit selectedTaskChanged(name);

    m_selecting = false;
}

void NimbleTaskStepWidget::onDataChanged(const QModelIndex &topLeft,
                                         const QModelIndex &bottomRight,
                                         const QVector<int> &roles)
{
    QTC_ASSERT(topLeft == bottomRight, return );
    if (!roles.contains(Qt::CheckStateRole))
        return;

    auto item = m_tasks.itemFromIndex(topLeft);
    if (!item)
        return;

    if (m_selecting)
        return;
    m_selecting = true;

    if (item->checkState() == Qt::Checked) {
        uncheckedAllDifferentFrom(item);
        emit selectedTaskChanged(item->text());
    } else if (item->checkState() == Qt::Unchecked) {
        emit selectedTaskChanged(QString());
    }

    m_selecting = false;
}

void NimbleTaskStepWidget::uncheckedAllDifferentFrom(QStandardItem *toSkip)
{
    for (int i = 0; i < m_tasks.rowCount(); ++i) {
        auto item = m_tasks.item(i);
        if (!item || item == toSkip)
            continue;
        item->setCheckState(Qt::Unchecked);
    }
}
