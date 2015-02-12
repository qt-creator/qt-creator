/**************************************************************************
**
** Copyright (c) nsf <no.smile.face@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "emacskeysstate.h"

#include <QTextCursor>
#include <QPlainTextEdit>

using namespace EmacsKeys::Internal;

//---------------------------------------------------------------------------
// EmacsKeysState
//---------------------------------------------------------------------------

EmacsKeysState::EmacsKeysState(QPlainTextEdit *edit):
    m_ignore3rdParty(false),
    m_mark(-1),
    m_lastAction(KeysAction3rdParty),
    m_editorWidget(edit)
{
    connect(edit, SIGNAL(cursorPositionChanged()),
        this, SLOT(cursorPositionChanged()));
    connect(edit, SIGNAL(textChanged()),
        this, SLOT(textChanged()));
    connect(edit, SIGNAL(selectionChanged()),
        this, SLOT(selectionChanged()));
}

EmacsKeysState::~EmacsKeysState() {}

void EmacsKeysState::setLastAction(EmacsKeysAction action)
{
    if (m_mark != -1) {
        // this code can be triggered only by 3rd party actions
        beginOwnAction();
        QTextCursor cursor = m_editorWidget->textCursor();
        cursor.clearSelection();
        m_editorWidget->setTextCursor(cursor);
        m_mark = -1;
        endOwnAction(action);
    } else {
        m_lastAction = action;
    }
}

void EmacsKeysState::cursorPositionChanged() {
    if (!m_ignore3rdParty)
        setLastAction(KeysAction3rdParty);
}

void EmacsKeysState::textChanged() {
    if (!m_ignore3rdParty)
        setLastAction(KeysAction3rdParty);
}

void EmacsKeysState::selectionChanged()
{
    if (!m_ignore3rdParty)
        setLastAction(KeysAction3rdParty);
}
