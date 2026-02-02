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

static QWidget *(*s_dialogParentGetter)() = nullptr;

void setDialogParentGetter(QWidget *(*getter)())
{
    s_dialogParentGetter = getter;
}

QWidget *dialogParent()
{
    return s_dialogParentGetter ? s_dialogParentGetter() : nullptr;
}

const char IGNORE_FOR_DIRTY_HOOK[] = "qtcIgnoreForDirtyHook";

bool setIgnoreForDirtyHook(QWidget *widget, bool ignore)
{
    const bool prev = widget->property(IGNORE_FOR_DIRTY_HOOK).toBool();
    if (prev != ignore)
        widget->setProperty(IGNORE_FOR_DIRTY_HOOK, ignore);
    return prev;
}

bool isIgnoredForDirtyHook(const QObject *object)
{
    return object->property(IGNORE_FOR_DIRTY_HOOK).toBool();
}

static std::function<void (bool)> s_markSettingDirtyHook;
static std::function<void ()> s_checkSettingDirtyHook;

namespace Internal {

void setMarkSettingsDirtyHook(const std::function<void (bool)> &hook)
{
    s_markSettingDirtyHook = hook;
}

void setCheckSettingsDirtyHook(const std::function<void ()> &hook)
{
    s_checkSettingDirtyHook = hook;
}

} // Internal

void markSettingsDirty(bool dirty)
{
    QTC_ASSERT(s_markSettingDirtyHook, return);
    s_markSettingDirtyHook(dirty);
}

void checkSettingsDirty()
{
    QTC_ASSERT(s_checkSettingDirtyHook, return);
    s_checkSettingDirtyHook();
}

} // namespace Utils
