/****************************************************************************
**
** Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
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

#include "completinglineedit.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QEvent>
#include <QKeyEvent>

namespace Utils {

CompletingLineEdit::CompletingLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
}

bool CompletingLineEdit::event(QEvent *e)
{
    // workaround for QTCREATORBUG-9453
    if (e->type() == QEvent::ShortcutOverride) {
        if (QCompleter *comp = completer()) {
            if (comp->popup() && comp->popup()->isVisible()) {
                QKeyEvent *ke = static_cast<QKeyEvent *>(e);
                if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
                    ke->accept();
                    return true;
                }
            }
        }
    }
    return QLineEdit::event(e);
}

void CompletingLineEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Down && !e->modifiers()) {
        if (QCompleter *comp = completer()) {
            if (!comp->popup()->isVisible())
                comp->complete();
        }
    }
    return QLineEdit::keyPressEvent(e);
}

} // namespace Utils
