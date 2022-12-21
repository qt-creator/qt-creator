// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/parser/qmljslexer_p.h>
#include <qmljs/parser/qmljsengine_p.h>

namespace Debugger::Internal {

class InteractiveInterpreter: QmlJS::Lexer
{
public:
    InteractiveInterpreter()
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

} // Debugger::Internal
