/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "progresspie.h"
#include "stylehelper.h"

#include <QtGui/QPainter>
#include <QtGui/QFont>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtDebug>

ProgressBar::ProgressBar(QWidget *parent)
    : QProgressBar(parent), m_error(false)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

ProgressBar::~ProgressBar()
{
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
    s.setHeight(fontMetrics().height() * 2);
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
    QProgressBar::mousePressEvent(event);
}

void ProgressBar::paintEvent(QPaintEvent *)
{
    // TODO move font into stylehelper
    // TODO use stylehelper white

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
    boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
    boldFont.setBold(true);
    p.setFont(boldFont);
    QFontMetrics fm(boldFont);

    // Draw separator
    int h = fm.height();
    p.setPen(QColor(0, 0, 0, 70));
    p.drawLine(0,0, size().width(), 0);

    p.setPen(QColor(255, 255, 255, 70));
    p.drawLine(0, 1, size().width(), 1);

    p.setPen(StyleHelper::panelTextColor());
    p.drawText(QPoint(7, h+1), m_title);

    m_progressHeight = h-4;
    m_progressHeight += ((m_progressHeight % 2) + 1) % 2; // make odd
    // draw outer rect
    QRect rect(INDENT, h+6, size().width()-2*INDENT-m_progressHeight+1, m_progressHeight-1);
    p.setPen(StyleHelper::panelTextColor());
    p.drawRect(rect);

    // draw inner rect
    QColor c = StyleHelper::panelTextColor();
    c.setAlpha(180);
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    QRect inner = rect.adjusted(2, 2, -1, -1);
    inner.adjust(0, 0, qRound((percent - 1) * inner.width()), 0);
    if (m_error) {
        // TODO this is not fancy enough
        QColor red(255, 0, 0, 180);
        p.setBrush(red);
        // avoid too small red bar
        if (inner.width() < 10)
            inner.adjust(0, 0, 10 - inner.width(), 0);
    } else if (value() == maximum()) {
        QColor green(140, 255, 140, 180);
        p.setBrush(green);
    }
    p.drawRect(inner);

    if (value() < maximum() && !m_error) {
        // draw cancel thingy
        // TODO this is quite ugly at the moment
        p.setPen(StyleHelper::panelTextColor());
        p.setBrush(QBrush(Qt::NoBrush));
        QRect cancelRect(size().width()-INDENT-m_progressHeight+3, h+6+1, m_progressHeight-3, m_progressHeight-3);
        p.drawRect(cancelRect);
        p.setPen(c);
        p.drawLine(cancelRect.center()+QPoint(-1,-1), cancelRect.center()+QPoint(+3,+3));
        p.drawLine(cancelRect.center()+QPoint(+3,-1), cancelRect.center()+QPoint(-1,+3));
    }
}
