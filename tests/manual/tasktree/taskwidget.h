// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/tasking/tasktree.h>

#include <utils/layoutbuilder.h>

#include <QWidget>

class StateIndicator;

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;
QT_END_NAMESPACE

enum class State {
    Initial,
    Running,
    Done,
    Error
};

enum class ExecuteMode {
    Sequential, // default
    Parallel
};

class StateWidget : public QWidget
{
public:
    StateWidget();

    void setState(State state);

protected:
    StateIndicator *m_stateIndicator = nullptr;
};

class TaskWidget : public StateWidget
{
public:
    TaskWidget();

    void setBusyTime(int seconds);
    int busyTime() const;
    void setSuccess(bool success);
    bool isSuccess() const;

private:
    QLabel *m_infoLabel = nullptr;
    QSpinBox *m_spinBox = nullptr;
    QCheckBox *m_checkBox = nullptr;
};

class GroupWidget : public StateWidget
{
public:
    GroupWidget();

    void setExecuteMode(ExecuteMode mode);
    Tasking::ParallelLimit executeMode() const;

    void setWorkflowPolicy(Tasking::WorkflowPolicy policy);
    Tasking::WorkflowPolicy workflowPolicy() const;

private:
    void updateExecuteMode();
    void updateWorkflowPolicy();

    QComboBox *m_executeCombo = nullptr;
    QComboBox *m_workflowCombo = nullptr;

    ExecuteMode m_executeMode = ExecuteMode::Sequential;
    Tasking::WorkflowPolicy m_workflowPolicy = Tasking::WorkflowPolicy::StopOnError;
};

class TaskGroup
{
public:
    QWidget *group;
    Layouting::Column items;
};

void createItem(Layouting::LayoutItem *item, const TaskGroup &taskGroup);
