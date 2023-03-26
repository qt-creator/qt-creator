// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QSpinBox)

namespace QmlDesigner {

namespace Ui {
class TimelineAnimationForm;
}

class TimelineAnimationForm : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineAnimationForm(QWidget *parent);
    ~TimelineAnimationForm() override;
    void setup(const ModelNode &animation);
    ModelNode animation() const;

private:
    void setupAnimation();

    void setAnimation(const ModelNode &animation);
    void setProperty(const PropertyName &propertyName, const QVariant &value);
    void connectSpinBox(QSpinBox *spinBox, const PropertyName &propertyName);
    void populateStateComboBox();

    Ui::TimelineAnimationForm *ui;

    QmlTimeline m_timeline;
    ModelNode m_animation;
};

} // namespace QmlDesigner
