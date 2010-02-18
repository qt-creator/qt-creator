/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "progresspie.h"

#include <utils/stylehelper.h>

#include <QtGui/QPainter>
#include <QtGui/QFont>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtDebug>
#define PROGRESSBAR_HEIGHT 11

ProgressBar::ProgressBar(QWidget *parent)
    : QWidget(parent), m_error(false), m_minimum(1), m_maximum(100), m_value(1)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setMouseTracking(true);
}

ProgressBar::~ProgressBar()
{
}

void ProgressBar::reset()
{
    m_value = m_minimum;
    update();
}

void ProgressBar::setRange(int minimum, int maximum)
{
    m_minimum = minimum;
    m_maximum = maximum;
    if (m_value < m_minimum || m_value > m_maximum)
        m_value = m_minimum;
    update();
}

void ProgressBar::setValue(int value)
{
    if (m_value == value
            || m_value < m_minimum
            || m_value > m_maximum) {
        return;
    }
    m_value = value;
    update();
}

QString ProgressBar::title() const
{
    return m_title;
}

bool ProgressBar::hasError() const
{
    return m_error;
}

void ProgressBar::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void ProgressBar::setError(bool on)
{
    m_error = on;
    update();
}

QSize ProgressBar::sizeHint() const
{
    QSize s;
    s.setWidth(50);
    s.setHeight(fontMetrics().height() + PROGRESSBAR_HEIGHT + 7);
    return s;
}

namespace { const int INDENT = 7; }

void ProgressBar::mousePressEvent(QMouseEvent *event)
{
    if (event->modifiers() == Qt::NoModifier
        && event->x() >= size().width()-INDENT-m_progressHeight) {
        event->accept();
        emit clicked();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ProgressBar::mouseMoveEvent(QMouseEvent *)
{
    update();
}

void ProgressBar::paintEvent(QPaintEvent *)
{
    // TODO move font into Utils::StyleHelper
    // TODO use Utils::StyleHelper white

    double range = maximum() - minimum();
    double percent = 0.50;
    if (range != 0)
        percent = (value() - minimum()) / range;
    if (percent > 1)
        percent = 1;
    else if (percent < 0)
        percent = 0;

    QPainter p(this);
    QFont boldFont(p.font());
    boldFont.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    p.setFont(boldFont);
    QFontMetrics fm(boldFont);

    // Draw separator
    int h = fm.height();
    p.setPen(Utils::StyleHelper::sidebarShadow());
    p.drawLine(0,0, size().width(), 0);

    p.setPen(Utils::StyleHelper::sidebarHighlight());
    p.drawLine(0, 1, size().width(), 1);

    QRect textRect = rect().adjusted(0, 0, -1, 0);
    textRect.setHeight(h+5);

    p.setPen(QColor(30, 30, 30, 80));
    p.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, m_title);
    p.translate(0, -1);
    p.setPen(Utils::StyleHelper::panelTextColor());
    p.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, m_title);
    p.translate(0, 1);

    m_progressHeight = PROGRESSBAR_HEIGHT;
    m_progressHeight += ((m_progressHeight % 2) + 1) % 2; // make odd
    // draw outer rect
    QRect rect(INDENT - 1, h+6, size().width()-2*INDENT, m_progressHeight-1);
    p.setPen(Utils::StyleHelper::panelTextColor());
    p.drawRect(rect);

    // draw inner rect
    QColor c = Utils::StyleHelper::panelTextColor();
    c.setAlpha(180);
    p.setPen(Qt::NoPen);

    QRect inner = rect.adjusted(2, 2, -1, -1);
    inner.adjust(0, 0, qRound((percent - 1) * inner.width()), 0);
    if (m_error) {
        QColor red(255, 60, 0, 210);
        c = red;
        // avoid too small red bar
        if (inner.width() < 10)
            inner.adjust(0, 0, 10 - inner.width(), 0);
    } else if (value() == maximum()) {
        c = QColor(120, 245, 90, 180);
    }

    QLinearGradient grad(inner.topLeft(), inner.bottomLeft());
    grad.setColorAt(0, c.lighter(114));
    grad.setColorAt(0.5, c.lighter(104));
    grad.setColorAt(0.51, c.darker(108));
    grad.setColorAt(1, c.darker(120));

    p.setBrush(grad);
    p.drawRect(inner);

    if (value() < maximum() && !m_error) {
        QColor cancelOutline = Utils::StyleHelper::panelTextColor();
        p.setPen(cancelOutline);
        QRect cancelRect(rect.right() - m_progressHeight + 2, rect.top(), m_progressHeight-1, rect.height());
        if (cancelRect.contains(mapFromGlobal(QCursor::pos())))
            p.setBrush(QColor(230, 90, 40, 190));
        else
            p.setBrush(Qt::NoBrush);

        p.drawRect(cancelRect);

        p.setPen(QPen(QColor(0, 0, 0, 70), 3));
        p.drawLine(cancelRect.center()+QPoint(-1,-1), cancelRect.center()+QPoint(+3,+3));
        p.drawLine(cancelRect.center()+QPoint(+3,-1), cancelRect.center()+QPoint(-1,+3));

        p.setPen(Utils::StyleHelper::panelTextColor());
        p.drawLine(cancelRect.center()+QPoint(-1,-1), cancelRect.center()+QPoint(+3,+3));
        p.drawLine(cancelRect.center()+QPoint(+3,-1), cancelRect.center()+QPoint(-1,+3));
    }
}
