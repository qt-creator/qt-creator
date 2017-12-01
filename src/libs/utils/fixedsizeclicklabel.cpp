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

#include "fixedsizeclicklabel.h"

#include <QMouseEvent>

/*!
    \class Utils::FixedSizeClickLabel

    \brief The FixedSizeClickLabel class is a label with a size hint derived from a sample text
    that can be different to the text that is shown.

    For convenience it also has a clicked signal that is emitted whenever the label receives a mouse
    click.
*/

namespace Utils {

FixedSizeClickLabel::FixedSizeClickLabel(QWidget *parent)
    : QLabel(parent)
    , m_pressed(false)
{
}

void FixedSizeClickLabel::setText(const QString &text, const QString &maxText)
{
    QLabel::setText(text);
    m_maxText = maxText;
}

QSize FixedSizeClickLabel::sizeHint() const
{
    return fontMetrics().boundingRect(m_maxText).size();
}

QString FixedSizeClickLabel::maxText() const
{
    return m_maxText;
}

void FixedSizeClickLabel::setMaxText(const QString &maxText)
{
    m_maxText = maxText;
}

void FixedSizeClickLabel::mousePressEvent(QMouseEvent *ev)
{
    QLabel::mousePressEvent(ev);
    if (ev->button() == Qt::LeftButton)
        m_pressed = true;
}

void FixedSizeClickLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    QLabel::mouseReleaseEvent(ev);
    if (ev->button() != Qt::LeftButton)
        return;
    if (m_pressed && rect().contains(ev->pos()))
        emit clicked();
    m_pressed = false;
}

} // namespace Utils
