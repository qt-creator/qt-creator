/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
