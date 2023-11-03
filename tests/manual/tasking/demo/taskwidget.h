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
    Success,
    Error,
    Canceled
};

enum class ExecuteMode {
    Sequential, // default
    Parallel
};

class StateWidget : public QWidget
{
public:
    StateWidget(State initialState = State::Initial);
    void setState(State state);

protected:
    StateIndicator *m_stateIndicator = nullptr;
};

class TaskWidget : public QWidget
{
public:
    TaskWidget();

    void setState(State state) { m_stateWidget->setState(state); }

    void setBusyTime(int seconds);
    int busyTime() const;
    void setSuccess(bool success);
    bool isSuccess() const;

private:
    StateWidget *m_stateWidget = nullptr;
    QLabel *m_infoLabel = nullptr;
    QSpinBox *m_spinBox = nullptr;
    QCheckBox *m_checkBox = nullptr;
};

class GroupWidget : public QWidget
{
public:
    GroupWidget();

    void setState(State state) { m_stateWidget->setState(state); }

    void setExecuteMode(ExecuteMode mode);
    Tasking::GroupItem executeMode() const;

    void setWorkflowPolicy(Tasking::WorkflowPolicy policy);
    Tasking::GroupItem workflowPolicy() const;

private:
    void updateExecuteMode();
    void updateWorkflowPolicy();

    StateWidget *m_stateWidget = nullptr;
    QComboBox *m_executeCombo = nullptr;
    QComboBox *m_workflowCombo = nullptr;

    ExecuteMode m_executeMode = ExecuteMode::Sequential;
    Tasking::WorkflowPolicy m_workflowPolicy = Tasking::WorkflowPolicy::StopOnError;
};

class StateLabel : public QWidget
{
public:
    StateLabel(State state);
};

#endif // TASKWIDGET_H
