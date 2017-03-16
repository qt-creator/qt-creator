/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qmljs/qmljsconstants.h>
QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Lexer;
class Directives;
class MemoryPool;

class QML_PARSER_EXPORT DiagnosticMessage
{
public:
    DiagnosticMessage()
        : kind(Severity::Error) {}

    DiagnosticMessage(Severity::Enum kind, const AST::SourceLocation &loc, const QString &message)
        : kind(kind), loc(loc), message(message) {}

    bool isWarning() const
    { return kind == Severity::Warning; }

    bool isError() const
    { return kind == Severity::Error; }

    Severity::Enum kind;
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
    const QString &code() const { return _code; }

    void addComment(int pos, int len, int line, int col);
    QList<AST::SourceLocation> comments() const;

    Lexer *lexer() const;
    void setLexer(Lexer *lexer);

    Directives *directives() const;
    void setDirectives(Directives *directives);

    MemoryPool *pool();

    inline QStringRef midRef(int position, int size) { return _code.midRef(position, size); }

    QStringRef newStringRef(const QString &s);
    QStringRef newStringRef(const QChar *chars, int size);
};

double integerFromString(const char *buf, int size, int radix);

} // end of namespace QmlJS

QT_QML_END_NAMESPACE

