/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "shortcutwidget.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>

namespace QmlDesigner {

ShortcutWidget::ShortcutWidget(QWidget *parent)
    : QWidget(parent)
    , m_text(new QLineEdit)
    , m_button(new QPushButton("R"))
    , m_key({{0, 0, 0, 0}})
    , m_keyNum(0)
{
    connect(m_button, &QPushButton::pressed, this, &ShortcutWidget::done);

    auto *box = new QHBoxLayout;
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget(m_text);
    box->addWidget(m_button);
    setLayout(box);

    m_text->setReadOnly(true);
    m_text->setFocusPolicy(Qt::NoFocus);
}

bool ShortcutWidget::containsFocus() const
{
    if (auto *fw = QApplication::focusWidget())
        if (fw->parentWidget() == this)
            return true;
    return false;
}

QString ShortcutWidget::text() const
{
    return m_text->text();
}

void ShortcutWidget::reset()
{
    emit cancel();
}

void ShortcutWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int width = event->size().width();
    m_text->setFixedWidth(width * 3 / 4);
    m_button->setFixedWidth(width / 4);
}

// Forked from qt-creator/shortcutsettings.cpp
static int translateModifiers(Qt::KeyboardModifiers state, const QString &text)
{
    int result = 0;
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key anyway
    if ((state & Qt::ShiftModifier)
        && (text.isEmpty() || !text.at(0).isPrint() || text.at(0).isLetterOrNumber()
            || text.at(0).isSpace()))
        result |= Qt::SHIFT;
    if (state & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (state & Qt::MetaModifier)
        result |= Qt::META;
    if (state & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}

void ShortcutWidget::recordKeysequence(QKeyEvent *event)
{
    int nextKey = event->key();
    if (m_keyNum > 3 || nextKey == Qt::Key_Control || nextKey == Qt::Key_Shift
        || nextKey == Qt::Key_Meta || nextKey == Qt::Key_Alt) {
        return;
    }

    nextKey |= translateModifiers(event->modifiers(), event->text());
    m_key[m_keyNum] = nextKey;
    m_keyNum++;
    event->accept();

    auto sequence = QKeySequence(m_key[0], m_key[1], m_key[2], m_key[3]);
    m_text->setText(sequence.toString());
}

} // namespace QmlDesigner.
