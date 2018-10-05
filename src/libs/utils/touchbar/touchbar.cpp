/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "touchbar.h"

namespace Utils {
namespace Internal {

class TouchBarPrivate
{
public:
    QByteArray m_id;
    QAction m_action;
};

} // namespace Internal

TouchBar::TouchBar(const QByteArray &id, const QIcon &icon, const QString &title)
    : d(new Internal::TouchBarPrivate)
{
    d->m_id = id;
    d->m_action.setIcon(icon);
    d->m_action.setText(title);
}

TouchBar::TouchBar(const QByteArray &id, const QIcon &icon)
    : TouchBar(id, icon, {})
{
}

TouchBar::TouchBar(const QByteArray &id, const QString &title)
    : TouchBar(id, {}, title)
{
}

TouchBar::TouchBar(const QByteArray &id)
    : TouchBar(id, {}, {})
{
}

TouchBar::~TouchBar()
{
    delete d;
}

QByteArray TouchBar::id() const
{
    return d->m_id;
}

QAction *TouchBar::touchBarAction() const
{
    return &d->m_action;
}

void TouchBar::insertAction(QAction *, const QByteArray &, QAction *)
{
}

void TouchBar::insertTouchBar(QAction *, TouchBar *)
{
}

void TouchBar::removeAction(QAction *)
{
}

void TouchBar::removeTouchBar(TouchBar *)
{
}

void TouchBar::clear()
{
}

void TouchBar::setApplicationTouchBar()
{
}

} // namespace Utils
