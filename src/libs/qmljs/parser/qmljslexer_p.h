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

#ifndef QMLJSLEXER_P_H
#define QMLJSLEXER_P_H

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
#include "qmljsgrammar_p.h"
#include <QString>

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Engine;

class QML_PARSER_EXPORT Directives {
public:
    virtual ~Directives() {}

    virtual void pragmaLibrary()
    {
    }

    virtual void importFile(const QString &jsfile, const QString &module)
    {
        Q_UNUSED(jsfile);
        Q_UNUSED(module);
    }

    virtual void importModule(const QString &uri, const QString &version, const QString &module)
    {
        Q_UNUSED(uri);
        Q_UNUSED(version);
        Q_UNUSED(module);
    }
};

class QML_PARSER_EXPORT Lexer: public QmlJSGrammar
{
public:
    enum {
        T_ABSTRACT = T_RESERVED_WORD,
        T_BOOLEAN = T_RESERVED_WORD,
        T_BYTE = T_RESERVED_WORD,
        T_CHAR = T_RESERVED_WORD,
        T_CLASS = T_RESERVED_WORD,
        T_DOUBLE = T_RESERVED_WORD,
        T_ENUM = T_RESERVED_WORD,
        T_EXPORT = T_RESERVED_WORD,
        T_EXTENDS = T_RESERVED_WORD,
        T_FINAL = T_RESERVED_WORD,
        T_FLOAT = T_RESERVED_WORD,
        T_GOTO = T_RESERVED_WORD,
        T_IMPLEMENTS = T_RESERVED_WORD,
        T_INT = T_RESERVED_WORD,
        T_INTERFACE = T_RESERVED_WORD,
        T_LET = T_RESERVED_WORD,
        T_LONG = T_RESERVED_WORD,
        T_NATIVE = T_RESERVED_WORD,
        T_PACKAGE = T_RESERVED_WORD,
        T_PRIVATE = T_RESERVED_WORD,
        T_PROTECTED = T_RESERVED_WORD,
        T_SHORT = T_RESERVED_WORD,
        T_STATIC = T_RESERVED_WORD,
        T_SUPER = T_RESERVED_WORD,
        T_SYNCHRONIZED = T_RESERVED_WORD,
        T_THROWS = T_RESERVED_WORD,
        T_TRANSIENT = T_RESERVED_WORD,
        T_VOLATILE = T_RESERVED_WORD,
        T_YIELD = T_RESERVED_WORD
    };

    enum Error {
        NoError,
        IllegalCharacter,
        UnclosedStringLiteral,
        IllegalEscapeSequence,
        IllegalUnicodeEscapeSequence,
        UnclosedComment,
        IllegalExponentIndicator,
        IllegalIdentifier
    };

    enum RegExpBodyPrefix {
        NoPrefix,
        EqualPrefix
    };

    enum RegExpFlag {
        RegExp_Global     = 0x01,
        RegExp_IgnoreCase = 0x02,
        RegExp_Multiline  = 0x04
    };

public:
    Lexer(Engine *engine);

    bool qmlMode() const;

    QString code() const;
    void setCode(const QString &code, int lineno, bool qmlMode = true);

    int lex();

    bool scanRegExp(RegExpBodyPrefix prefix = NoPrefix);
    bool scanDirectives(Directives *directives);

    int regExpFlags() const { return _patternFlags; }
    QString regExpPattern() const { return _tokenText; }

    int tokenKind() const;
    int tokenOffset() const;
    int tokenLength() const;

    int tokenStartLine() const;
    int tokenStartColumn() const;

    int tokenEndLine() const;
    int tokenEndColumn() const;

    QStringRef tokenSpell() const;
    double tokenValue() const;
    QString tokenText() const;

    Error errorCode() const;
    QString errorMessage() const;

    bool prevTerminator() const;
    bool followsClosingBrace() const;
    bool canInsertAutomaticSemicolon(int token) const;

    enum ParenthesesState {
        IgnoreParentheses,
        CountParentheses,
        BalancedParentheses
    };

protected:
    int classify(const QChar *s, int n, bool qmlMode);

private:
    inline void scanChar();
    int scanToken();

    bool isLineTerminator() const;
    static bool isIdentLetter(QChar c);
    static bool isDecimalDigit(ushort c);
    static bool isHexDigit(QChar c);
    static bool isOctalDigit(ushort c);
    static bool isUnicodeEscapeSequence(const QChar *chars);

    void syncProhibitAutomaticSemicolon();
    QChar decodeUnicodeEscapeCharacter(bool *ok);

private:
    Engine *_engine;

    QString _code;
    QString _tokenText;
    QString _errorMessage;
    QStringRef _tokenSpell;

    const QChar *_codePtr;
    const QChar *_lastLinePtr;
    const QChar *_tokenLinePtr;
    const QChar *_tokenStartPtr;

    QChar _char;
    Error _errorCode;

    int _currentLineNumber;
    double _tokenValue;

    // parentheses state
    ParenthesesState _parenthesesState;
    int _parenthesesCount;

    int _stackToken;

    int _patternFlags;
    int _tokenKind;
    int _tokenLength;
    int _tokenLine;

    bool _validTokenText;
    bool _prohibitAutomaticSemicolon;
    bool _restrictedKeyword;
    bool _terminator;
    bool _followsClosingBrace;
    bool _delimited;
    bool _qmlMode;
};

} // end of namespace QmlJS

QT_QML_END_NAMESPACE

#endif // LEXER_H
