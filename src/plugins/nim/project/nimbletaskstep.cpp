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

#include "nimbletaskstep.h"

#include "nimconstants.h"
#include "nimblebuildsystem.h"
#include "nimbleproject.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardPaths>

using namespace ProjectExplorer;

namespace Nim {

class NimbleTaskStepWidget : public BuildStepConfigWidget
{
    Q_DECLARE_TR_FUNCTIONS(Nim::NimbleTaskStep)

public:
    explicit NimbleTaskStepWidget(NimbleTaskStep *buildStep);

    void selectedTaskChanged(const QString &name)
    {
        auto taskStep = dynamic_cast<NimbleTaskStep *>(step());
        QTC_ASSERT(taskStep, return);
        taskStep->setTaskName(name);
    }

private:
    void updateTaskList();

    void selectTask(const QString &name);

    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

    void uncheckedAllDifferentFrom(QStandardItem *item);

    QStandardItemModel m_tasks;
    bool m_selecting = false;
};

NimbleTaskStepWidget::NimbleTaskStepWidget(NimbleTaskStep *bs)
    : BuildStepConfigWidget(bs)
{
    auto taskArgumentsLineEdit = new QLineEdit(this);
    taskArgumentsLineEdit->setText(bs->taskArgs());

    auto taskList = new QListView(this);
    taskList->setFrameShape(QFrame::StyledPanel);
    taskList->setSelectionMode(QAbstractItemView::NoSelection);
    taskList->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskList->setModel(&m_tasks);

    auto formLayout = new QFormLayout(this);
    formLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    formLayout->addRow(tr("Task arguments:"), taskArgumentsLineEdit);
    formLayout->addRow(tr("Tasks:"), taskList);

    auto buildSystem = dynamic_cast<NimbleBuildSystem *>(bs->buildSystem());
    QTC_ASSERT(buildSystem, return);

    connect(&m_tasks, &QAbstractItemModel::dataChanged, this, &NimbleTaskStepWidget::onDataChanged);

    updateTaskList();
    connect(buildSystem, &NimbleBuildSystem::tasksChanged, this, &NimbleTaskStepWidget::updateTaskList);

    selectTask(bs->taskName());
    connect(bs, &NimbleTaskStep::taskNameChanged, this, &NimbleTaskStepWidget::selectTask);
    connect(bs, &NimbleTaskStep::taskNameChanged, this, &NimbleTaskStepWidget::recreateSummary);

    connect(bs, &NimbleTaskStep::taskArgsChanged, taskArgumentsLineEdit, &QLineEdit::setText);
    connect(bs, &NimbleTaskStep::taskArgsChanged, this, &NimbleTaskStepWidget::recreateSummary);
    connect(taskArgumentsLineEdit, &QLineEdit::textChanged, bs ,&NimbleTaskStep::setTaskArgs);

    setSummaryUpdater([this, bs] {
        return QString("<b>%1:</b> nimble %2 %3")
                .arg(displayName(), bs->taskName(), bs->taskArgs());
    });
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

    QList<QStandardItem *> items = m_tasks.findItems(name);
    QStandardItem *item = items.empty() ? nullptr : items.front();
    uncheckedAllDifferentFrom(item);
    if (item)
        item->setCheckState(Qt::Checked);

    selectedTaskChanged(name);

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
        selectedTaskChanged(item->text());
    } else if (item->checkState() == Qt::Unchecked) {
        selectedTaskChanged(QString());
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

NimbleTaskStep::NimbleTaskStep(BuildStepList *parentList, Utils::Id id)
    : AbstractProcessStep(parentList, id)
{
    setDefaultDisplayName(tr(Constants::C_NIMBLETASKSTEP_DISPLAY));
    setDisplayName(tr(Constants::C_NIMBLETASKSTEP_DISPLAY));
}

bool NimbleTaskStep::init()
{
    processParameters()->setEnvironment(buildEnvironment());
    processParameters()->setWorkingDirectory(project()->projectDirectory());
    return validate() && AbstractProcessStep::init();
}

BuildStepConfigWidget *NimbleTaskStep::createConfigWidget()
{
    return new NimbleTaskStepWidget(this);
}

bool NimbleTaskStep::fromMap(const QVariantMap &map)
{
    setTaskName(map.value(Constants::C_NIMBLETASKSTEP_TASKNAME, QString()).toString());
    setTaskArgs(map.value(Constants::C_NIMBLETASKSTEP_TASKARGS, QString()).toString());
    return validate() ? AbstractProcessStep::fromMap(map) : false;
}

QVariantMap NimbleTaskStep::toMap() const
{
    QVariantMap result = AbstractProcessStep::toMap();
    result[Constants::C_NIMBLETASKSTEP_TASKNAME] = taskName();
    result[Constants::C_NIMBLETASKSTEP_TASKARGS] = taskArgs();
    return result;
}

void NimbleTaskStep::setTaskName(const QString &name)
{
    if (m_taskName == name)
        return;
    m_taskName = name;
    emit taskNameChanged(name);
    updateCommandLine();
}

void NimbleTaskStep::setTaskArgs(const QString &args)
{
    if (m_taskArgs == args)
        return;
    m_taskArgs = args;
    emit taskArgsChanged(args);
    updateCommandLine();
}

void NimbleTaskStep::updateCommandLine()
{
    QString args = m_taskName + " " + m_taskArgs;
    Utils::CommandLine commandLine(Utils::FilePath::fromString(QStandardPaths::findExecutable("nimble")),
                                   args, Utils::CommandLine::Raw);

    processParameters()->setCommandLine(commandLine);
}

bool NimbleTaskStep::validate()
{
    if (m_taskName.isEmpty())
        return true;

    auto nimbleBuildSystem = dynamic_cast<NimbleBuildSystem*>(buildSystem());
    QTC_ASSERT(nimbleBuildSystem, return false);

    if (!Utils::contains(nimbleBuildSystem->tasks(), [this](const NimbleTask &task){ return task.name == m_taskName; })) {
        emit addTask(BuildSystemTask(Task::Error, tr("Nimble task %1 not found.").arg(m_taskName)));
        emitFaultyConfigurationMessage();
        return false;
    }

    return true;
}

// Factory

NimbleTaskStepFactory::NimbleTaskStepFactory()
{
    registerStep<NimbleTaskStep>(Constants::C_NIMBLETASKSTEP_ID);
    setDisplayName(NimbleTaskStep::tr("Nimble Task"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN,
                           ProjectExplorer::Constants::BUILDSTEPS_DEPLOY});
    setSupportedConfiguration(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setRepeatable(true);
}

} // Nim
