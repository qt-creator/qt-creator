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

#include "execmenu.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QMenu>
#include <QPoint>
#include <QRect>
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
    QRect screen = qApp->desktop()->availableGeometry(widget);
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

} // namespace Utils
