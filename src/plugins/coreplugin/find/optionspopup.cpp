/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "optionspopup.h"

#include <coreplugin/actionmanager/actionmanager.h>

#include <utils/qtcassert.h>

#include <QAction>
#include <QCheckBox>
#include <QEvent>
#include <QKeyEvent>
#include <QVBoxLayout>

using namespace Utils;

namespace Core {

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
        QCheckBox *checkBox = createCheckboxForCommand(command);
        if (first) {
            checkBox->setFocus();
            first = false;
        }
        layout->addWidget(checkBox);
    }
    move(parent->mapToGlobal(QPoint(0, -sizeHint().height())));
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

void OptionsPopup::actionChanged()
{
    auto action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    QCheckBox *checkbox = m_checkboxMap.value(action);
    QTC_ASSERT(checkbox, return);
    checkbox->setEnabled(action->isEnabled());
}

QCheckBox *OptionsPopup::createCheckboxForCommand(Id id)
{
    QAction *action = ActionManager::command(id)->action();
    QCheckBox *checkbox = new QCheckBox(action->text());
    checkbox->setToolTip(action->toolTip());
    checkbox->setChecked(action->isChecked());
    checkbox->setEnabled(action->isEnabled());
    checkbox->installEventFilter(this); // enter key handling
    QObject::connect(checkbox, &QCheckBox::clicked, action, &QAction::setChecked);
    QObject::connect(action, &QAction::changed, this, &OptionsPopup::actionChanged);
    m_checkboxMap.insert(action, checkbox);
    return checkbox;
}

} // namespace Core
