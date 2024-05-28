// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "execmenu.h"

#include "tooltip/tooltip.h"

#include <QMenu>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QWidget>

namespace Utils {

/*!
 * Opens \a menu at the specified \a widget position.
 * This function computes the position where to show the menu, and opens it with
 * QMenu::exec().
 */
QAction *execMenuAtWidget(QMenu *menu, QWidget *widget)
{
    QPoint p;
    QRect screen = widget->screen()->availableGeometry();
    QSize sh = menu->sizeHint();
    QRect rect = widget->rect();
    if (widget->isRightToLeft()) {
        if (widget->mapToGlobal(QPoint(0, rect.bottom())).y() + sh.height() <= screen.height())
            p = widget->mapToGlobal(rect.bottomRight());
        else
            p = widget->mapToGlobal(rect.topRight() - QPoint(0, sh.height()));
        p.rx() -= sh.width();
    } else {
        if (widget->mapToGlobal(QPoint(0, rect.bottom())).y() + sh.height() <= screen.height())
            p = widget->mapToGlobal(rect.bottomLeft());
        else
            p = widget->mapToGlobal(rect.topLeft() - QPoint(0, sh.height()));
    }
    p.rx() = qMax(screen.left(), qMin(p.x(), screen.right() - sh.width()));
    p.ry() += 1;

    return menu->exec(p);
}

/*!
    Adds tool tips to the \a menu that show the action's tool tip when hovering
    over an entry.
 */
void addToolTipsToMenu(QMenu *menu)
{
    QObject::connect(menu, &QMenu::hovered, menu, [menu](QAction *action) {
        ToolTip::show(menu->mapToGlobal(menu->actionGeometry(action).topRight()), action->toolTip());
    });
}

} // namespace Utils
