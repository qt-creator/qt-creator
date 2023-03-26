// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QSpinBox)

namespace QmlDesigner {

namespace Ui {
class TimelineForm;
}

class TimelineForm : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineForm(QWidget *parent);
    ~TimelineForm() override;
    void setTimeline(const QmlTimeline &timeline);
    QmlTimeline timeline() const;
    void setHasAnimation(bool b);

private:
    void setProperty(const PropertyName &propertyName, const QVariant &value);
    void connectSpinBox(QSpinBox *spinBox, const PropertyName &propertyName);

    Ui::TimelineForm *ui;
    QmlTimeline m_timeline;
};

} // namespace QmlDesigner
