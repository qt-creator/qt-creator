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

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QThread>

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
template<typename CallableType>
class CallableEvent : public QEvent
{
public:
    using Callable = std::decay_t<CallableType>;
    CallableEvent(Callable &&callable)
        : QEvent(QEvent::None)
        , callable(std::move(callable))
    {}
    CallableEvent(const Callable &callable)
        : QEvent(QEvent::None)
        , callable(callable)
    {}

    ~CallableEvent() { callable(); }

public:
    Callable callable;
};

template<typename Callable>
void executeInLoop(Callable &&callable, QObject *object = QCoreApplication::instance())
{
    if (QThread *thread = qobject_cast<QThread *>(object))
        object = QAbstractEventDispatcher::instance(thread);

    QCoreApplication::postEvent(object,
                                new CallableEvent<Callable>(std::forward<Callable>(callable)),
                                Qt::HighEventPriority);
}
#else
template<typename Callable>
void executeInLoop(Callable &&callable, QObject *object = QCoreApplication::instance())
{
    if (QThread *thread = qobject_cast<QThread *>(object))
        object = QAbstractEventDispatcher::instance(thread);

    QMetaObject::invokeMethod(object, std::forward<Callable>(callable));
}
#endif
