/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "palettebox.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>

namespace qmt {

PaletteBox::PaletteBox(QWidget *parent)
    : QWidget(parent),
      m_brushes(6),
      m_pens(6)
{
    setFocusPolicy(Qt::StrongFocus);
}

PaletteBox::~PaletteBox()
{
}

QBrush PaletteBox::brush(int index) const
{
    QMT_ASSERT(index >= 0 && index <= m_brushes.size(), return QBrush());
    return m_brushes.at(index);
}

void PaletteBox::setBrush(int index, const QBrush &brush)
{
    QMT_ASSERT(index >= 0 && index <= m_brushes.size(), return);
    if (m_brushes[index] != brush) {
        m_brushes[index] = brush;
        update();
    }
}

QPen PaletteBox::linePen(int index) const
{
    QMT_ASSERT(index >= 0 && index <= m_pens.size(), return QPen());
    return m_pens.at(index);
}

void PaletteBox::setLinePen(int index, const QPen &pen)
{
    QMT_ASSERT(index >= 0 && index <= m_pens.size(), return);
    if (m_pens[index] != pen) {
        m_pens[index] = pen;
        update();
    }
}

void PaletteBox::clear()
{
    setCurrentIndex(-1);
}

void PaletteBox::setCurrentIndex(int index)
{
    if (m_currentIndex != index) {
        if (index >= 0 && index < m_brushes.size())
            m_currentIndex = index;
        else
            m_currentIndex = -1;
        update();
    }
}

void PaletteBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal w = static_cast<qreal>(width()) / static_cast<qreal>(m_brushes.size());
    qreal h = height();
    for (int i = 0; i < m_brushes.size(); ++i) {
        QBrush brush = m_brushes.at(i);
        if (i == m_currentIndex) {
            painter.fillRect(QRectF(i * w, 0, w, h), brush);
            QPen pen = m_pens.at(i);
            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawRect(QRectF(i * w + 1, 1, w - 2, h - 2));
        } else {
            painter.fillRect(QRectF(i * w, 0, w, h), brush);
        }
    }
    if (hasFocus()) {
        painter.setBrush(Qt::NoBrush);
        QPen pen;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.drawRect(0, 0, width() - 1, height() - 1);
    }
}

void PaletteBox::mousePressEvent(QMouseEvent *event)
{
    qreal w = static_cast<qreal>(width()) / static_cast<qreal>(m_brushes.size());

    int i = static_cast<int>((event->x() / w));
    QMT_ASSERT(i >= 0 && i < m_brushes.size(), return);
    setCurrentIndex(i);
    if (m_currentIndex >= 0 && m_currentIndex < m_brushes.size())
        emit activated(m_currentIndex);
}

void PaletteBox::keyPressEvent(QKeyEvent *event)
{
    bool isKnownKey = false;
    switch (event->key()) {
    case Qt::Key_Left:
        if (m_currentIndex <= 0)
            setCurrentIndex((m_brushes.size() - 1));
        else
            setCurrentIndex(m_currentIndex - 1);
        isKnownKey = true;
        break;
    case Qt::Key_Right:
        if (m_currentIndex < 0 || m_currentIndex >= m_brushes.size() - 1)
            setCurrentIndex(0);
        else
            setCurrentIndex(m_currentIndex + 1);
        isKnownKey = true;
        break;
    }
    if (isKnownKey && m_currentIndex >= 0 && m_currentIndex < m_brushes.size())
        emit activated(m_currentIndex);
}

} // namespace qmt
