// Copyright (C) 2023 Tasuku Suzuki <tasuku.suzuki@signal-slot.co.jp>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "guiutils.h"

#include "aspects.h"
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
#include <QRadioButton>
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

bool setIgnoreForDirtyHook(QObject *object, bool ignore)
{
    const bool prev = object->property(IGNORE_FOR_DIRTY_HOOK).toBool();
    if (prev != ignore)
        object->setProperty(IGNORE_FOR_DIRTY_HOOK, ignore);
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
    // This can happen if per-project settings are opened before
    // the settings mode was entered. The hook is not installed at
    // that time, and it's ok to do nothing.
    if (!s_markSettingDirtyHook)
        return;

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

static void installDirtyTriggerHelper(QObject *object, bool check)
{
    QTC_ASSERT(object, return);

    const auto action = check ? checkSettingsDirty : markSettingsDirty;

    // Keep this before QAbstractButton
    if (auto ob = qobject_cast<QtColorButton *>(object)) {
        QObject::connect(ob, &QtColorButton::colorChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QAbstractButton *>(object)) {
        QObject::connect(ob, &QAbstractButton::pressed, action);
        return;
    }
    if (auto ob = qobject_cast<QLineEdit *>(object)) {
        QObject::connect(ob, &QLineEdit::textChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QComboBox *>(object)) {
        QObject::connect(ob, &QComboBox::currentIndexChanged, action);
        QObject::connect(ob, &QComboBox::currentTextChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QSpinBox *>(object)) {
        QObject::connect(ob, &QSpinBox::valueChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QPlainTextEdit *>(object)) {
        QObject::connect(ob, &QPlainTextEdit::textChanged, action);
        return;
    }
    if (auto ob = qobject_cast<PlainTextEdit *>(object)) {
        QObject::connect(ob, &PlainTextEdit::textChanged, action);
        return;
    }
    if (auto ob = qobject_cast<QAbstractItemModel *>(object)) {
        QObject::connect(ob, &QAbstractItemModel::rowsInserted, action);
        QObject::connect(ob, &QAbstractItemModel::rowsRemoved, action);
        return;
    }
    if (auto ob = qobject_cast<BaseAspect *>(object)) {
        QObject::connect(ob, &BaseAspect::volatileValueChanged, action);
        return;
    }

    QTC_CHECK(false);
}

void installMarkSettingsDirtyTrigger(QObject *object)
{
    installDirtyTriggerHelper(object, false);
}

void installCheckSettingsDirtyTrigger(QObject *object)
{
    installDirtyTriggerHelper(object, true);
}

bool suppressSettingsDirtyTrigger(bool suppress)
{
    const bool prev = s_suppressSettingsDirtyTrigger;
    s_suppressSettingsDirtyTrigger = suppress;
    return prev;
}

void installMarkSettingsDirtyTriggerRecursively(QObject *object)
{
    QTC_ASSERT(object, return);

    if (isIgnoredForDirtyHook(object))
        return;

    QList<QObject *> children = {object};

    while (!children.isEmpty()) {
        QObject *child = children.takeLast();
        if (isIgnoredForDirtyHook(child))
            continue;

        children += child->findChildren<QObject *>(Qt::FindDirectChildrenOnly);

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
        if (auto ob = qobject_cast<QRadioButton *>(child)) {
            QObject::connect(ob, &QRadioButton::toggled, markDirty);
            continue;
        }
    }
}

} // namespace Utils
