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

#include "elidinglabel.h"
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

/*!
    \class Utils::ElidingLabel

    \brief The ElidingLabel class is a label suitable for displaying elided
    text.
*/

namespace Utils {

ElidingLabel::ElidingLabel(QWidget *parent)
    : QLabel(parent), m_elideMode(Qt::ElideRight)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred, QSizePolicy::Label));
}

ElidingLabel::ElidingLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent), m_elideMode(Qt::ElideRight)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred, QSizePolicy::Label));
}

Qt::TextElideMode ElidingLabel::elideMode() const
{
    return m_elideMode;
}

void ElidingLabel::setElideMode(const Qt::TextElideMode &elideMode)
{
    m_elideMode = elideMode;
    update();
}

void ElidingLabel::paintEvent(QPaintEvent *)
{
    const int m = margin();
    QRect contents = contentsRect().adjusted(m, m, -m, -m);
    QFontMetrics fm = fontMetrics();
    QString txt = text();
    if (txt.length() > 4 && fm.width(txt) > contents.width()) {
        setToolTip(txt);
        txt = fm.elidedText(txt, m_elideMode, contents.width());
    } else {
        setToolTip(QString());
    }
    int flags = QStyle::visualAlignment(layoutDirection(), alignment()) | Qt::TextSingleLine;

    QPainter painter(this);
    drawFrame(&painter);
    painter.drawText(contents, flags, txt);
}

} // namespace Utils
