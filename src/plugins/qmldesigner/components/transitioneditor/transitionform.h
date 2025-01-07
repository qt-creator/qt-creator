// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QSpinBox)

namespace QmlDesigner {

namespace Ui {
class TransitionForm;
}

class TransitionForm : public QWidget
{
    Q_OBJECT

public:
    explicit TransitionForm(QWidget *parent);
    ~TransitionForm() override;
    void setTransition(const ModelNode &transition);
    ModelNode transition() const;
    ModelNode stateGroupNode() const;

signals:
    void stateGroupChanged(const ModelNode &transition, const ModelNode &stateGroup);

private:
    void setupStatesLists();
    void setupStateGroups();

    Ui::TransitionForm *ui;
    ModelNode m_transition;
};

} // namespace QmlDesigner
