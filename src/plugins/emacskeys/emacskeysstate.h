/****************************************************************************
**
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

#pragma once

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)

namespace EmacsKeys {
namespace Internal {

enum EmacsKeysAction {
    KeysAction3rdParty,
    KeysActionKillWord,
    KeysActionKillLine,
    KeysActionOther,
};

class EmacsKeysState : public QObject
{
public:
    EmacsKeysState(QPlainTextEdit *edit);
    ~EmacsKeysState();
    void setLastAction(EmacsKeysAction action);
    void beginOwnAction() { m_ignore3rdParty = true; }
    void endOwnAction(EmacsKeysAction action) {
        m_ignore3rdParty = false;
        m_lastAction = action;
    }
    EmacsKeysAction lastAction() const { return m_lastAction; }

    int mark() const { return m_mark; }
    void setMark(int mark) { m_mark = mark; }

private:
    void cursorPositionChanged();
    void textChanged();
    void selectionChanged();

    bool m_ignore3rdParty;
    int m_mark;
    EmacsKeysAction m_lastAction;
    QPlainTextEdit *m_editorWidget;
};

} // namespace Internal
} // namespace EmacsKeys
