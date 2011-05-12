/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
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

#include <QString>
#include <QSet>

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {
class QML_PARSER_EXPORT NameId
{
    QString _text;

public:
    NameId(const QChar *u, int s)
        : _text(u, s)
    { }

    const QString asString() const
    { return _text; }

    bool operator == (const NameId &other) const
    { return _text == other._text; }

    bool operator != (const NameId &other) const
    { return _text != other._text; }

    bool operator < (const NameId &other) const
    { return _text < other._text; }
};

uint qHash(const QmlJS::NameId &id);

} // end of namespace QmlJS

namespace QmlJS {

class Lexer;
class NodePool;

namespace Ecma {

class QML_PARSER_EXPORT RegExp
{
public:
    enum RegExpFlag {
        Global     = 0x01,
        IgnoreCase = 0x02,
        Multiline  = 0x04
    };

public:
    static int flagFromChar(const QChar &);
    static QString flagsToString(int flags);
};

} // end of namespace Ecma

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
    NodePool *_nodePool;
    QSet<NameId> _literals;
    QList<QmlJS::AST::SourceLocation> _comments;

public:
    Engine();
    ~Engine();

    QSet<NameId> literals() const;

    void addComment(int pos, int len, int line, int col);
    QList<QmlJS::AST::SourceLocation> comments() const;

    NameId *intern(const QChar *u, int s);

    static QString toString(NameId *id);

    Lexer *lexer() const;
    void setLexer(Lexer *lexer);

    NodePool *nodePool() const;
    void setNodePool(NodePool *nodePool);
};

} // end of namespace QmlJS

QT_QML_END_NAMESPACE

#endif // QMLJSENGINE_P_H
