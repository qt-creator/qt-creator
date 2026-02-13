// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "guiutils.h"

#include "hostosinfo.h"
#include "pathchooser.h"
#include "plaintextedit/plaintextedit.h"
#include "qtcolorbutton.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSpinBox>
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

static bool s_suppressSettingsDirtyTrigger = false;

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

void markSettingsDirty()
{
    QTC_ASSERT(s_markSettingDirtyHook, return);
    if (s_suppressSettingsDirtyTrigger)
        return;
    s_markSettingDirtyHook(true);
}

void checkSettingsDirty()
{
    QTC_ASSERT(s_checkSettingDirtyHook, return);
    if (s_suppressSettingsDirtyTrigger)
        return;
    s_checkSettingDirtyHook();
}

static void installDirtyTriggerHelper(QWidget *widget, bool check)
{
    QTC_ASSERT(widget, return);

    const auto action = check ? checkSettingsDirty : markSettingsDirty;

    // Keep this before QAbstractButton
    if (auto ob = qobject_cast<QtColorButton *>(widget)) {
        QObject::connect(ob, &QtColorButton::colorChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QAbstractButton *>(widget)) {
        QObject::connect(ob, &QAbstractButton::pressed, action);
        return;
    }
    if (auto ob = qobject_cast<QLineEdit *>(widget)) {
        QObject::connect(ob, &QLineEdit::textChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QComboBox *>(widget)) {
        QObject::connect(ob, &QComboBox::currentIndexChanged, action);
        QObject::connect(ob, &QComboBox::currentTextChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QSpinBox *>(widget)) {
        QObject::connect(ob, &QSpinBox::valueChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QPlainTextEdit *>(widget)) {
        QObject::connect(ob, &QPlainTextEdit::textChanged, action);
        return;
    }
    if (auto ob = qobject_cast<PlainTextEdit *>(widget)) {
        QObject::connect(ob, &PlainTextEdit::textChanged, action);
        return;
    }

    QTC_CHECK(false);
}

void installMarkSettingsDirtyTrigger(QWidget *widget)
{
    installDirtyTriggerHelper(widget, false);
}

void installCheckSettingsDirtyTrigger(QWidget *widget)
{
    installDirtyTriggerHelper(widget, true);
}

bool suppressSettingsDirtyTrigger(bool suppress)
{
    const bool prev = s_suppressSettingsDirtyTrigger;
    s_suppressSettingsDirtyTrigger = suppress;
    return prev;
}

void installMarkSettingsDirtyTriggerRecursively(QWidget *widget)
{
    QTC_ASSERT(widget, return);

    if (isIgnoredForDirtyHook(widget))
        return;

    QList<QWidget *> children = {widget};

    while (!children.isEmpty()) {
        QWidget *child = children.takeLast();
        if (isIgnoredForDirtyHook(child))
            continue;

        children += child->findChildren<QWidget *>(Qt::FindDirectChildrenOnly);

        if (child->metaObject() == &QWidget::staticMetaObject)
            continue;

        if (child->metaObject() == &QLabel::staticMetaObject)
            continue;

        if (child->metaObject() == &QScrollBar::staticMetaObject)
            continue;

        if (child->metaObject() == &QMenu::staticMetaObject)
            continue;

        auto markDirty = [child] {
            if (isIgnoredForDirtyHook(child))
                return;
            markSettingsDirty();
        };

        if (auto ob = qobject_cast<QLineEdit *>(child)) {
            QObject::connect(ob, &QLineEdit::textEdited, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QComboBox *>(child)) {
            QObject::connect(ob, &QComboBox::currentIndexChanged, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QSpinBox *>(child)) {
            QObject::connect(ob, &QSpinBox::valueChanged, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QGroupBox *>(child)) {
            QObject::connect(ob, &QGroupBox::toggled, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QCheckBox *>(child)) {
            QObject::connect(ob, &QCheckBox::toggled, markDirty);
            continue;
        }
        if (auto ob = qobject_cast<QListWidget *>(child)) {
            QObject::connect(ob, &QListWidget::itemChanged, markDirty);
            QObject::connect(ob->model(), &QAbstractItemModel::rowsInserted, markDirty);
            QObject::connect(ob->model(), &QAbstractItemModel::rowsRemoved, markDirty);
            continue;
        }
    }
}

} // namespace Utils
