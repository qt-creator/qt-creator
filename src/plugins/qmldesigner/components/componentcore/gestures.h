/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include <QGesture>
#include <QGestureRecognizer>
#include <QLineF>

QT_FORWARD_DECLARE_CLASS(QTouchEvent)

namespace QmlDesigner {

class TwoFingerSwipe : public QGesture
{
    Q_OBJECT

public:
    TwoFingerSwipe();

    static Qt::GestureType type();
    static void registerRecognizer();

    QPointF direction() const;

    void reset();
    QGestureRecognizer::Result begin(QTouchEvent *event);
    QGestureRecognizer::Result update(QTouchEvent *event);
    QGestureRecognizer::Result end(QTouchEvent *event);

private:
    static Qt::GestureType m_type;

    QLineF m_start;
    QLineF m_current;
    QLineF m_last;
};

class TwoFingerSwipeRecognizer : public QGestureRecognizer
{
public:
    TwoFingerSwipeRecognizer();

    QGesture *create(QObject *target) override;

    QGestureRecognizer::Result recognize(QGesture *gesture, QObject *watched, QEvent *event) override;

    void reset(QGesture *gesture) override;
};

} // namespace QmlDesigner
