// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskwidget.h"

#include <utils/layoutbuilder.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>

using namespace Utils;
using namespace Layouting;

static QString colorButtonStyleSheet(const QColor &bgColor)
{
    QString rc("border-width: 2px; border-radius: 2px; border-color: black; ");
    rc += bgColor.isValid() ? "border-style: solid; background:" + bgColor.name() + ";"
                            : QString("border-style: dotted;");
    return rc;
}

static QColor stateToColor(State state) {
    switch (state) {
    case State::Initial: return Qt::gray;
    case State::Running: return Qt::yellow;
    case State::Done: return Qt::green;
    case State::Error: return Qt::red;
    }
    return {};
}

class StateIndicator : public QLabel
{
public:
    StateIndicator()
    {
        m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Small);
        m_progressIndicator->attachToWidget(this);
        m_progressIndicator->hide();
        updateState();
    }

    void setState(State state)
    {
        if (m_state == state)
            return;
        m_state = state;
        updateState();
    }

private:
    void updateState()
    {
        setStyleSheet(colorButtonStyleSheet(stateToColor(m_state)));
        if (m_state == State::Running)
            m_progressIndicator->show();
        else
            m_progressIndicator->hide();
    }
    State m_state = State::Initial;
    ProgressIndicator *m_progressIndicator = nullptr;
};

StateWidget::StateWidget()
    : m_stateIndicator(new StateIndicator)
{
}

void StateWidget::setState(State state)
{
    m_stateIndicator->setState(state);
}

TaskWidget::TaskWidget()
    : m_infoLabel(new QLabel("Sleep:"))
    , m_spinBox(new QSpinBox)
    , m_checkBox(new QCheckBox("Report success"))
{
    m_stateIndicator->setFixedSize(30, 30);
    m_spinBox->setSuffix(" sec");
    setBusyTime(1);
    setSuccess(true);

    Row {
        m_stateIndicator,
        m_infoLabel,
        m_spinBox,
        m_checkBox,
        st
    }.attachTo(this, WithoutMargins);
}

void TaskWidget::setBusyTime(int seconds)
{
    m_spinBox->setValue(seconds);
}

int TaskWidget::busyTime() const
{
    return m_spinBox->value();
}

void TaskWidget::setSuccess(bool success)
{
    m_checkBox->setChecked(success);
}

bool TaskWidget::isSuccess() const
{
    return m_checkBox->isChecked();
}

GroupWidget::GroupWidget()
    : m_executeCombo(new QComboBox)
    , m_workflowCombo(new QComboBox)
{
    m_stateIndicator->setFixedWidth(30);

    m_executeCombo->addItem("Sequential", (int)Tasking::ExecuteMode::Sequential);
    m_executeCombo->addItem("Parallel", (int)Tasking::ExecuteMode::Parallel);
    updateExecuteMode();
    connect(m_executeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_executeMode = (Tasking::ExecuteMode)m_executeCombo->itemData(index).toInt();
    });

    m_workflowCombo->addItem("Stop On Error", (int)Tasking::WorkflowPolicy::StopOnError);
    m_workflowCombo->addItem("Cont On Error", (int)Tasking::WorkflowPolicy::ContinueOnError);
    m_workflowCombo->addItem("Stop On Done", (int)Tasking::WorkflowPolicy::StopOnDone);
    m_workflowCombo->addItem("Cont On Done", (int)Tasking::WorkflowPolicy::ContinueOnDone);
    m_workflowCombo->addItem("Optional", (int)Tasking::WorkflowPolicy::Optional);
    updateWorkflowPolicy();
    connect(m_workflowCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_workflowPolicy = (Tasking::WorkflowPolicy)m_workflowCombo->itemData(index).toInt();
    });

    Row {
        m_stateIndicator,
        Column {
            new QLabel("Execute:"),
            m_executeCombo,
            new QLabel("Workflow:"),
            m_workflowCombo,
            st
        }
    }.attachTo(this, WithoutMargins);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}

void GroupWidget::setExecuteMode(Tasking::ExecuteMode mode)
{
    m_executeMode = mode;
    updateExecuteMode();
}

void GroupWidget::updateExecuteMode()
{
    m_executeCombo->setCurrentIndex(m_executeCombo->findData((int)m_executeMode));
}

Tasking::ExecuteMode GroupWidget::executeMode() const
{
    return m_executeMode;
}

void GroupWidget::setWorkflowPolicy(Tasking::WorkflowPolicy policy)
{
    m_workflowPolicy = policy;
    updateWorkflowPolicy();
}

void GroupWidget::updateWorkflowPolicy()
{
    m_workflowCombo->setCurrentIndex(m_workflowCombo->findData((int)m_workflowPolicy));
}

Tasking::WorkflowPolicy GroupWidget::workflowPolicy() const
{
    return m_workflowPolicy;
}

TaskGroup::TaskGroup(QWidget *group, std::initializer_list<LayoutBuilder::LayoutItem> items)
    : Row({ group, Group { Column { items } } }) {}

