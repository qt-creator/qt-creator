// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionspopup.h"

#include "../actionmanager/actionmanager.h"

#include <utils/qtcassert.h>

#include <QAction>
#include <QCheckBox>
#include <QEvent>
#include <QKeyEvent>
#include <QScreen>
#include <QVBoxLayout>

using namespace Utils;

namespace Core {

static QCheckBox *createCheckboxForCommand(QObject *owner, Id id)
{
    QAction *action = ActionManager::command(id)->action();
    QCheckBox *checkbox = new QCheckBox(action->text());
    checkbox->setToolTip(action->toolTip());
    checkbox->setChecked(action->isChecked());
    checkbox->setEnabled(action->isEnabled());
    checkbox->installEventFilter(owner); // enter key handling
    QObject::connect(checkbox, &QCheckBox::clicked, action, &QAction::setChecked);
    QObject::connect(action, &QAction::changed, checkbox, [action, checkbox] {
        checkbox->setEnabled(action->isEnabled());
    });
    return checkbox;
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
        QCheckBox *checkBox = createCheckboxForCommand(this, command);
        if (first) {
            checkBox->setFocus();
            first = false;
        }
        layout->addWidget(checkBox);
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

} // namespace Core
