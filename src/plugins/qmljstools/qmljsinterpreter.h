/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
