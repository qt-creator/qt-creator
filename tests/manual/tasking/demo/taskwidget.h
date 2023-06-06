// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TASKWIDGET_H
#define TASKWIDGET_H

#include <tasking/tasktree.h>

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
    Tasking::GroupItem executeMode() const;

    void setWorkflowPolicy(Tasking::WorkflowPolicy policy);
    Tasking::GroupItem workflowPolicy() const;

private:
    void updateExecuteMode();
    void updateWorkflowPolicy();

    QComboBox *m_executeCombo = nullptr;
    QComboBox *m_workflowCombo = nullptr;

    ExecuteMode m_executeMode = ExecuteMode::Sequential;
    Tasking::WorkflowPolicy m_workflowPolicy = Tasking::WorkflowPolicy::StopOnError;
};

#endif // TASKWIDGET_H
