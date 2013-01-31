/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "originwidget.h"
#include <QList>
#include <QPainter>
#include <QMouseEvent>
#include <qdeclarative.h>

static QList<QPoint> positions;
static QStringList originsStringList;

namespace QmlDesigner {

OriginWidget::OriginWidget(QWidget *parent) : QWidget(parent), m_pressed(false), m_marked(false)
{
    if (positions.isEmpty())
        positions << QPoint(0,  0) << QPoint(18,  0) << QPoint(36,  0)
            << QPoint(0,  18) << QPoint(18, 18) << QPoint(36, 18)
            << QPoint(0,  36) << QPoint(18, 36) << QPoint(36, 36);

    if (originsStringList.isEmpty())
        originsStringList << "TopLeft" << "Top"
                << "TopRight" << "Left" << "Center" << "Right"
                << "BottomLeft" << "Bottom" << "BottomRight";

    m_originString = "Center";
    resize(50, 50);
    setMinimumHeight(50);
    m_index = 0;
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
}

void OriginWidget::setOrigin(const QString& newOrigin)
{
    if (!originsStringList.contains(newOrigin))
        return;
    if (newOrigin == m_originString)
        return;

    m_originString = newOrigin;
    update();
    emit originChanged();
}

void OriginWidget::registerDeclarativeType()
{
    qmlRegisterType<QmlDesigner::OriginWidget>("Bauhaus",1,0,"OriginWidget");

}

void OriginWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    foreach (const QPoint& position, positions)
        p.fillRect(position.x(), position.y(), 14, 14, Qt::black);

    int origin = originsStringList.indexOf(m_originString);

    if (m_pressed)
        p.fillRect(positions.at(m_index).x() + 4, positions.at(m_index).y() + 4, 6, 6, "#868686");

    if (m_marked)
        p.fillRect(positions.at(origin).x(), positions.at(origin).y(), 14, 14, "#9999ff");
    else
        p.fillRect(positions.at(origin).x(), positions.at(origin).y(), 14, 14, "#e6e6e6");
    p.fillRect(positions.at(origin).x() + 2, positions.at(origin).y() + 2, 10, 10, "#666666");
}

void OriginWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = false;
        for (int i = 0; i < positions.size(); i++)
            if (QRect(positions.at(i), QSize(14, 14)).contains(event->pos()))
                setOrigin(originsStringList.at(i));
    }
    QWidget::mouseReleaseEvent(event);
}

void OriginWidget::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        for (int i = 0; i < positions.size(); i++)
            if (QRect(positions.at(i), QSize(14, 14)).contains(event->pos())) {
            m_index = i;
            update();
        }
    }
    QWidget::mousePressEvent(event);
}

static inline QString getToolTip(const QPoint &pos)
{
    for (int i = 0; i < positions.size(); i++)
        if (QRect(positions.at(i), QSize(14, 14)).contains(pos))
            return originsStringList.at(i);

    return QString();
}

bool OriginWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
        setToolTip(getToolTip(static_cast<QHelpEvent*>(event)->pos()));

    return QWidget::event(event);
}

} //QmlDesigner

