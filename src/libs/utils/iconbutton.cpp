// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iconbutton.h"

#include "stylehelper.h"
#include "theme/theme.h"

#include <QEnterEvent>
#include <QEvent>
#include <QPainter>

namespace Utils {

IconButton::IconButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setAttribute(Qt::WA_Hover);
}

void IconButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter p(this);
    QRect r(QPoint(), size());

    if (m_containsMouse && isEnabled()) {
        QColor c = creatorTheme()->color(Theme::TextColorDisabled);
        c.setAlphaF(c.alphaF() * .5);
        StyleHelper::drawPanelBgRect(&p, r, c);
    }

    icon().paint(&p, r, Qt::AlignCenter);
}

void IconButton::enterEvent(QEnterEvent *e)
{
    m_containsMouse = true;
    e->accept();
    update();
}

void IconButton::leaveEvent(QEvent *e)
{
    m_containsMouse = false;
    e->accept();
    update();
}

QSize IconButton::sizeHint() const
{
    QWindow *window = this->window()->windowHandle();
    QSize s = icon().actualSize(window, QSize(32, 16)) + QSize(8, 8);

    if (StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyleRelaxed)
        s += QSize(5, 5);

    return s;
}

} // namespace Utils
