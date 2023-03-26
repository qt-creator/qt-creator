// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "touchbar.h"

#include <QAction>
#include <QByteArray>
#include <QIcon>
#include <QString>

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
