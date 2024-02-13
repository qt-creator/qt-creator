// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "guiutils.h"
#include "hostosinfo.h"

#include <QEvent>
#include <QGuiApplication>
#include <QWidget>

namespace Utils {

namespace Internal {

static bool isWheelModifier()
{
    return QGuiApplication::keyboardModifiers()
           == (HostOsInfo::isMacHost() ? Qt::MetaModifier : Qt::ControlModifier);
}

class WheelEventFilter : public QObject
{
public:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Wheel && !isWheelModifier()) {
            QWidget *widget = qobject_cast<QWidget *>(watched);
            if (widget && widget->focusPolicy() != Qt::WheelFocus && !widget->hasFocus()) {
                QObject *parent = widget->parentWidget();
                if (parent)
                    return parent->event(event);
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

} // namespace Internal

void QTCREATOR_UTILS_EXPORT setWheelScrollingWithoutFocusBlocked(QWidget *widget)
{
    static Internal::WheelEventFilter instance;
    // Installing duplicated event filter for the same objects just brings the event filter
    // to the front and is otherwise no-op (the second event filter isn't installed).
    widget->installEventFilter(&instance);
    if (widget->focusPolicy() == Qt::WheelFocus)
        widget->setFocusPolicy(Qt::StrongFocus);
}

} // namespace Utils
