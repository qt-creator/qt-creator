// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "pushbutton.h"

#include <QPainter>
#include <QStyleOptionButton>
#include <QDebug>
#include <QStylePainter>

namespace ADS {

QSize PushButton::sizeHint() const
{
    QSize sh = QPushButton::sizeHint();

    if (m_orientation != PushButton::Horizontal)
        sh.transpose();

    return sh;
}

PushButton::Orientation PushButton::buttonOrientation() const
{
    return m_orientation;
}

void PushButton::setButtonOrientation(Orientation orientation)
{
    m_orientation = orientation;
    updateGeometry();
}

void PushButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QStylePainter painter(this);
    QStyleOptionButton option;
    initStyleOption(&option);

    if (m_orientation == PushButton::VerticalTopToBottom) {
        painter.rotate(90);
        painter.translate(0, -1 * width());
        option.rect = option.rect.transposed();
    } else if (m_orientation == PushButton::VerticalBottomToTop) {
        painter.rotate(-90);
        painter.translate(-1 * height(), 0);
        option.rect = option.rect.transposed();
    }

    painter.drawControl(QStyle::CE_PushButton, option);
}

} // namespace ADS
