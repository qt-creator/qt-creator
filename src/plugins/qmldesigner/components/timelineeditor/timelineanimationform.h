// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class TimelineAnimationForm : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineAnimationForm(QWidget *parent);
    void setup(const ModelNode &animation);
    ModelNode animation() const;

private:
    void setupAnimation();

    void setAnimation(const ModelNode &animation);
    void setProperty(const PropertyName &propertyName, const QVariant &value);
    void connectSpinBox(QSpinBox *spinBox, const PropertyName &propertyName);
    void populateStateComboBox();

    QLineEdit *m_idLineEdit;
    QCheckBox *m_running;
    QSpinBox *m_startFrame;
    QSpinBox *m_endFrame;
    QSpinBox *m_duration;
    QCheckBox *m_continuous;
    QSpinBox *m_loops;
    QCheckBox *m_pingPong;
    QComboBox *m_transitionToState;

    QmlTimeline m_timeline;
    ModelNode m_animation;
};

} // namespace QmlDesigner
