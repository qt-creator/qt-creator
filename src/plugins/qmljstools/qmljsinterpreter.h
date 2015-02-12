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

#ifndef QMLJSINTERPRETER_H
#define QMLJSINTERPRETER_H

#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QVector>
#include <QString>
#include <QList>

namespace QmlJSTools {
namespace Internal {

class QmlJSInterpreter: QmlJS::Lexer
{
public:
    QmlJSInterpreter()
        : Lexer(&m_engine),
          m_stateStack(128)
    {

    }

    void clearText() { m_code.clear(); }
    void appendText(const QString &text) { m_code += text; }

    QString code() const { return m_code; }
    bool canEvaluate();

private:
    QmlJS::Engine m_engine;
    QVector<int> m_stateStack;
    QList<int> m_tokens;
    QString m_code;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSINTERPRETER_H
