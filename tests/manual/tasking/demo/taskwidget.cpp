// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "progressindicator.h"
#include "taskwidget.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QMetaEnum>
#include <QSpinBox>

using namespace Tasking;

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
    case State::Success: return Qt::green;
    case State::Error: return Qt::red;
    case State::Canceled: return Qt::cyan;
    }
    return {};
}

class StateIndicator : public QLabel
{
public:
    StateIndicator(State initialState = State::Initial, QWidget *parent = nullptr)
        : QLabel(parent)
        , m_state(initialState)
    {
        setAlignment(Qt::AlignCenter);
        QFont f = font();
        f.setBold(true);
        setFont(f);
        m_progressIndicator = new ProgressIndicator(this);
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
        setText(m_state == State::Canceled ? "X" : "");
    }
    State m_state = State::Initial;
    ProgressIndicator *m_progressIndicator = nullptr;
};

StateWidget::StateWidget(State initialState)
    : m_stateIndicator(new StateIndicator(initialState, this))
{
    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stateIndicator);
    setFixedSize(30, 30);
}

void StateWidget::setState(State state)
{
    m_stateIndicator->setState(state);
}

TaskWidget::TaskWidget()
    : m_stateWidget(new StateWidget)
    , m_infoLabel(new QLabel("Sleep:"))
    , m_spinBox(new QSpinBox)
    , m_checkBox(new QCheckBox("Report success"))
{
    m_spinBox->setSuffix(" sec");
    setBusyTime(1);
    setSuccess(true);

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_stateWidget);
    layout->addWidget(m_infoLabel);
    layout->addWidget(m_spinBox);
    layout->addWidget(m_checkBox);
    layout->addStretch();
    layout->setContentsMargins(0, 0, 0, 0);
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
    : m_stateWidget(new StateWidget)
    , m_executeCombo(new QComboBox)
    , m_workflowCombo(new QComboBox)
{
    m_stateWidget->setFixedSize(30, QWIDGETSIZE_MAX);

    m_executeCombo->addItem("Sequential", int(ExecuteMode::Sequential));
    m_executeCombo->addItem("Parallel", int(ExecuteMode::Parallel));
    updateExecuteMode();
    connect(m_executeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_executeMode = (ExecuteMode)m_executeCombo->itemData(index).toInt();
    });

    const QMetaEnum workflow = QMetaEnum::fromType<WorkflowPolicy>();
    for (int i = 0; i < workflow.keyCount(); ++i)
        m_workflowCombo->addItem(workflow.key(i), workflow.value(i));

    updateWorkflowPolicy();
    connect(m_workflowCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_workflowPolicy = (WorkflowPolicy)m_workflowCombo->itemData(index).toInt();
    });

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_stateWidget);
    QBoxLayout *subLayout = new QVBoxLayout;
    subLayout->addWidget(new QLabel("Execute Mode:"));
    subLayout->addWidget(m_executeCombo);
    subLayout->addWidget(new QLabel("Workflow Policy:"));
    subLayout->addWidget(m_workflowCombo);
    subLayout->addStretch();
    layout->addLayout(subLayout);
    layout->setContentsMargins(0, 0, 0, 0);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}

void GroupWidget::setExecuteMode(ExecuteMode mode)
{
    m_executeMode = mode;
    updateExecuteMode();
}

void GroupWidget::updateExecuteMode()
{
    m_executeCombo->setCurrentIndex(m_executeCombo->findData((int)m_executeMode));
}

GroupItem GroupWidget::executeMode() const
{
    return m_executeMode == ExecuteMode::Sequential ? sequential : parallel;
}

void GroupWidget::setWorkflowPolicy(WorkflowPolicy policy)
{
    m_workflowPolicy = policy;
    updateWorkflowPolicy();
}

void GroupWidget::updateWorkflowPolicy()
{
    m_workflowCombo->setCurrentIndex(m_workflowCombo->findData((int)m_workflowPolicy));
}

GroupItem GroupWidget::workflowPolicy() const
{
    return Tasking::workflowPolicy(m_workflowPolicy);
}

static QString stateToString(State state)
{
    switch (state) {
    case State::Initial : return "Initial";
    case State::Running : return "Running";
    case State::Success : return "Success";
    case State::Error : return "Error";
    case State::Canceled : return "Canceled";
    }
    return "";
}

StateLabel::StateLabel(State state)
{
    QBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(new StateWidget(state));
    layout->addWidget(new QLabel(stateToString(state)));
}
