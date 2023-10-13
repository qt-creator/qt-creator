// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "guiutils.h"

#include <QEvent>
#include <QWidget>

namespace Utils {

namespace Internal {

class WheelEventFilter : public QObject
{
public:
    bool eventFilter(QObject *watched, QEvent *event) override {
        auto widget = qobject_cast<QWidget *>(watched);
        return event->type() == QEvent::Wheel
               && widget
               && widget->focusPolicy() != Qt::WheelFocus
               && !widget->hasFocus();
    }
};

} // namespace Internal

void QTCREATOR_UTILS_EXPORT attachWheelBlocker(QWidget *widget)
{
    static Internal::WheelEventFilter instance;
    widget->installEventFilter(&instance);
    widget->setFocusPolicy(Qt::StrongFocus);
}

} // namespace Utils
