/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "linecolumnlabel.h"

#include <QMouseEvent>

/*!
    \class Utils::LineColumnLabel

    \brief The LineColumnLabel class is a label suitable for displaying cursor
    positions, and so on, with a fixed
    width derived from a sample text.
*/

namespace Utils {

LineColumnLabel::LineColumnLabel(QWidget *parent)
    : QLabel(parent)
    , m_pressed(false)
{
}

void LineColumnLabel::setText(const QString &text, const QString &maxText)
{
    QLabel::setText(text);
    m_maxText = maxText;
}

QSize LineColumnLabel::sizeHint() const
{
    return fontMetrics().boundingRect(m_maxText).size();
}

QString LineColumnLabel::maxText() const
{
    return m_maxText;
}

void LineColumnLabel::setMaxText(const QString &maxText)
{
    m_maxText = maxText;
}

void LineColumnLabel::mousePressEvent(QMouseEvent *ev)
{
    QLabel::mousePressEvent(ev);
    if (ev->button() == Qt::LeftButton)
        m_pressed = true;
}

void LineColumnLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    QLabel::mouseReleaseEvent(ev);
    if (ev->button() != Qt::LeftButton)
        return;
    if (m_pressed && rect().contains(ev->pos()))
        emit clicked();
    m_pressed = false;
}

} // namespace Utils
