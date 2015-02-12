/**************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "qmljsinterpreter.h"

namespace QmlJSTools {
namespace Internal {

bool QmlJSInterpreter::canEvaluate()
{
    int yyaction = 0;
    int yytoken = -1;
    int yytos = -1;

    setCode(m_code, 1);
    m_tokens.append(T_FEED_JS_PROGRAM);

    do {
        if (++yytos == m_stateStack.size())
            m_stateStack.resize(m_stateStack.size() * 2);

        m_stateStack[yytos] = yyaction;

again:
        if (yytoken == -1 && action_index[yyaction] != -TERMINAL_COUNT) {
            if (m_tokens.isEmpty())
                yytoken = lex();
            else
                yytoken = m_tokens.takeFirst();
        }

        yyaction = t_action(yyaction, yytoken);
        if (yyaction > 0) {
            if (yyaction == ACCEPT_STATE) {
                --yytos;
                return true;
            }
            yytoken = -1;
        } else if (yyaction < 0) {
            const int ruleno = -yyaction - 1;
            yytos -= rhs[ruleno];
            yyaction = nt_action(m_stateStack[yytos], lhs[ruleno] - TERMINAL_COUNT);
        }
    } while (yyaction);

    const int errorState = m_stateStack[yytos];
    if (t_action(errorState, T_AUTOMATIC_SEMICOLON) && canInsertAutomaticSemicolon(yytoken)) {
        yyaction = errorState;
        m_tokens.prepend(yytoken);
        yytoken = T_SEMICOLON;
        goto again;
    }

    if (yytoken != EOF_SYMBOL)
        return true;

    return false;
}

} // namespace Internal
} // namespace QmlJSTools
