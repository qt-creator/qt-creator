// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QRadioButton;
class QSpinBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class TimelineForm : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineForm(QWidget *parent);
    void setTimeline(const QmlTimeline &timeline);
    QmlTimeline timeline() const;
    void setHasAnimation(bool b);

private:
    void setProperty(const PropertyName &propertyName, const QVariant &value);
    void connectSpinBox(QSpinBox *spinBox, const PropertyName &propertyName);

    QLineEdit *m_idLineEdit;
    QSpinBox *m_startFrame;
    QSpinBox *m_endFrame;
    QRadioButton *m_expressionBinding;
    QRadioButton *m_animation;
    QLineEdit *m_expressionBindingLineEdit;

    QmlTimeline m_timeline;
};

} // namespace QmlDesigner
