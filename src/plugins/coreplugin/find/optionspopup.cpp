// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionspopup.h"

#include "../actionmanager/actionmanager.h"

#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QAction>
#include <QCheckBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QScreen>
#include <QSpinBox>
#include <QStyleOption>
#include <QVBoxLayout>

using namespace Utils;

const char kNumericOptionProperty[] = "qtc_numericOption";

namespace Core {

static QCheckBox *createCheckboxForAction(QObject *owner, QAction *action)
{
    QCheckBox *checkbox = new QCheckBox(action->text());
    checkbox->setToolTip(action->toolTip());
    checkbox->setChecked(action->isChecked());
    checkbox->setEnabled(action->isEnabled());
    checkbox->installEventFilter(owner); // enter key handling
    QObject::connect(checkbox, &QCheckBox::clicked, action, &QAction::setChecked);
    QObject::connect(action, &QAction::enabledChanged, checkbox, &QCheckBox::setEnabled);
    return checkbox;
}

static QWidget *createSpinboxForAction(QObject *owner, QAction *action)
{
    const std::optional<NumericOption> option = NumericOption::get(action);
    QTC_ASSERT(option, return nullptr);
    const auto proxyAction = qobject_cast<ProxyAction *>(action);
    QTC_ASSERT(proxyAction, return nullptr);

    const auto prefix = action->text().section("{}", 0, 0);
    const auto suffix = action->text().section("{}", 1);

    const auto widget = new QWidget{};
    widget->setEnabled(action->isEnabled());

    auto styleOption = QStyleOptionButton{};
    const QRect box = widget->style()->subElementRect(QStyle::SE_CheckBoxContents, &styleOption);

    const auto spinbox = new QSpinBox{widget};
    spinbox->installEventFilter(owner); // enter key handling
    spinbox->setMinimum(option->minimumValue);
    spinbox->setMaximum(option->maximumValue);
    spinbox->setValue(option->currentValue);

    QObject::connect(spinbox, &QSpinBox::valueChanged, action, [action](int value) {
        std::optional<NumericOption> option = NumericOption::get(action);
        QTC_ASSERT(option, return);
        option->currentValue = value;
        NumericOption::set(action, *option);
        emit action->changed();
    });
    const auto updateCurrentAction = [proxyAction] {
        if (!proxyAction->action())
            return;
        const std::optional<NumericOption> option = NumericOption::get(proxyAction);
        QTC_ASSERT(option, return);
        NumericOption::set(proxyAction->action(), *option);
        emit proxyAction->action()->changed();
    };
    QObject::connect(proxyAction, &ProxyAction::currentActionChanged, updateCurrentAction);
    QObject::connect(proxyAction, &QAction::changed, updateCurrentAction);
    QObject::connect(action, &QAction::enabledChanged, widget, &QWidget::setEnabled);

    const auto layout = new QHBoxLayout{widget};
    layout->setContentsMargins(box.left(), 0, 0, 0);
    layout->setSpacing(0); // the label will have spaces before and after the placeholder

    if (!prefix.isEmpty()) {
        const auto prefixLabel = new QLabel{prefix, widget};
        const auto prefixSpan = suffix.isEmpty() ? 1 : 0;
        layout->addWidget(prefixLabel, prefixSpan);
        prefixLabel->setBuddy(spinbox);
    }

    layout->addWidget(spinbox);

    if (!suffix.isEmpty()) {
        const auto suffixLabel = new QLabel{suffix, widget};
        layout->addWidget(suffixLabel, 1);
        suffixLabel->setBuddy(spinbox);
    }

    return widget;
}

static QWidget *createWidgetForCommand(QObject *owner, Id id)
{
    const auto action = ActionManager::command(id)->action();

    if (NumericOption::get(action))
        return createSpinboxForAction(owner, action);
    else
        return createCheckboxForAction(owner, action);
}

/*!
    \class Core::OptionsPopup
    \inmodule QtCreator
    \internal
*/

OptionsPopup::OptionsPopup(QWidget *parent, const QVector<Id> &commands)
    : QWidget(parent, Qt::Popup)
{
    setAttribute(Qt::WA_DeleteOnClose);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    setLayout(layout);

    bool first = true;
    for (const Id &command : commands) {
        const auto widget = createWidgetForCommand(this, command);
        if (first) {
            widget->setFocus();
            first = false;
        }
        layout->addWidget(widget);
    }
    const QPoint globalPos = parent->mapToGlobal(QPoint(0, -sizeHint().height()));
    const QRect screenGeometry = parent->screen()->availableGeometry();
    move(globalPos.x(), std::max(globalPos.y(), screenGeometry.y()));
}

bool OptionsPopup::event(QEvent *ev)
{
    if (ev->type() == QEvent::ShortcutOverride) {
        auto ke = static_cast<QKeyEvent *>(ev);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ev->accept();
            return true;
        }
    }
    return QWidget::event(ev);
}

bool OptionsPopup::eventFilter(QObject *obj, QEvent *ev)
{
    auto checkbox = qobject_cast<QCheckBox *>(obj);
    if (ev->type() == QEvent::KeyPress && checkbox) {
        auto ke = static_cast<QKeyEvent *>(ev);
        if (!ke->modifiers() && (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)) {
            checkbox->click();
            ev->accept();
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

std::optional<NumericOption> NumericOption::get(QObject *o)
{
    const QVariant opt = o->property(kNumericOptionProperty);
    if (opt.isValid()) {
        QTC_ASSERT(opt.canConvert<NumericOption>(), return {});
        return opt.value<NumericOption>();
    }
    return {};
}

void NumericOption::set(QObject *o, const NumericOption &opt)
{
    o->setProperty(kNumericOptionProperty, QVariant::fromValue(opt));
}

} // namespace Core
