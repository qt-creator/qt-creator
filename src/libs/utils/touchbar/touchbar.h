// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils_global.h"

QT_BEGIN_NAMESPACE
class QAction;
class QByteArray;
class QIcon;
class QString;
QT_END_NAMESPACE

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
