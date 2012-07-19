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

#ifndef QMLJSENGINE_P_H
#define QMLJSENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmljsglobal_p.h"
#include "qmljsastfwd_p.h"
#include "qmljsmemorypool_p.h"

#include <QString>
#include <QSet>

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Lexer;
class Directives;
class MemoryPool;

class QML_PARSER_EXPORT DiagnosticMessage
{
public:
    enum Kind { Warning, Error };

    DiagnosticMessage()
        : kind(Error) {}

    DiagnosticMessage(Kind kind, const AST::SourceLocation &loc, const QString &message)
        : kind(kind), loc(loc), message(message) {}

    bool isWarning() const
    { return kind == Warning; }

    bool isError() const
    { return kind == Error; }

    Kind kind;
    AST::SourceLocation loc;
    QString message;
};

class QML_PARSER_EXPORT Engine
{
    Lexer *_lexer;
    Directives *_directives;
    MemoryPool _pool;
    QList<AST::SourceLocation> _comments;
    QString _extraCode;
    QString _code;

public:
    Engine();
    ~Engine();

    void setCode(const QString &code);

    void addComment(int pos, int len, int line, int col);
    QList<AST::SourceLocation> comments() const;

    Lexer *lexer() const;
    void setLexer(Lexer *lexer);

    void setDirectives(Directives *directives);
    Directives *directives() const;

    MemoryPool *pool();

    inline QStringRef midRef(int position, int size) { return _code.midRef(position, size); }

    QStringRef newStringRef(const QString &s);
    QStringRef newStringRef(const QChar *chars, int size);
};

double integerFromString(const char *buf, int size, int radix);

} // end of namespace QmlJS

QT_QML_END_NAMESPACE

#endif // QMLJSENGINE_P_H
