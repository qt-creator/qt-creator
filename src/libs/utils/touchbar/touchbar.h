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

#pragma once

#include <utils/utils_global.h>

#include <QAction>
#include <QByteArray>
#include <QIcon>
#include <QString>

namespace Utils {

namespace Internal {
class TouchBarPrivate;
}

class QTCREATOR_UTILS_EXPORT TouchBar
{
public:
    TouchBar(const QByteArray &id, const QIcon &icon, const QString &title);
    TouchBar(const QByteArray &id, const QIcon &icon);
    TouchBar(const QByteArray &id, const QString &title);
    TouchBar(const QByteArray &id);

    ~TouchBar();

    QByteArray id() const;

    QAction *touchBarAction() const;

    void insertAction(QAction *before, const QByteArray &id, QAction *action);
    void insertTouchBar(QAction *before, TouchBar *touchBar);

    void removeAction(QAction *action);
    void removeTouchBar(TouchBar *touchBar);
    void clear();

    void setApplicationTouchBar();

private:
    Internal::TouchBarPrivate *d;
};

} // namespace Utils
