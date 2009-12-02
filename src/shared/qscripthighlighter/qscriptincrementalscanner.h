#ifndef QSCRIPTINCREMENTALSCANNER_H
#define QSCRIPTINCREMENTALSCANNER_H

#include <qscripthighlighter/qscripthighlighter_global.h>

#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>

namespace SharedTools {

class QSCRIPTHIGHLIGHTER_EXPORT QScriptIncrementalScanner
{
public:

    struct Token {
        int offset;
        int length;
        enum Kind {
            Keyword,
            Identifier,
            String,
            Comment,
            Number,
            LeftParenthesis,
            RightParenthesis,
            LeftBrace,
            RightBrace,
            LeftBracket,
            RightBracket,
            Operator,
            Semicolon,
            Colon,
            Comma,
            Dot
        } kind;

        inline Token(int o, int l, Kind k): offset(o), length(l), kind(k) {}
        inline int end() const { return offset + length; }
        inline bool is(int k) const { return k == kind; }
        inline bool isNot(int k) const { return k != kind; }
    };

public:
    QScriptIncrementalScanner();
    virtual ~QScriptIncrementalScanner();

    void setKeywords(const QSet<QString> keywords)
    { m_keywords = keywords;; }

    void reset();

    QList<QScriptIncrementalScanner::Token> operator()(const QString &text, int startState = 0);

    int endState() const
    { return m_endState; }

    int firstNonSpace() const
    { return m_firstNonSpace; }

    QList<QScriptIncrementalScanner::Token> tokens() const
    { return m_tokens; }

private:
    void blockEnd(int state, int firstNonSpace)
    { m_endState = state; m_firstNonSpace = firstNonSpace; }
    void insertString(int start)
    { insertToken(start, 1, Token::String, false); }
    void insertComment(int start, int length)
    { insertToken(start, length, Token::Comment, false); }
    void insertCharToken(int start, const char c);
    void insertIdentifier(int start)
    { insertToken(start, 1, Token::Identifier, false); }
    void insertNumber(int start)
    { insertToken(start, 1, Token::Number, false); }
    void insertToken(int start, int length, Token::Kind kind, bool forceNewToken);
    void scanForKeywords(const QString &text);

private:
    QSet<QString> m_keywords;
    int m_endState;
    int m_firstNonSpace;
    QList<QScriptIncrementalScanner::Token> m_tokens;
};

} // namespace SharedTools

#endif // QSCRIPTINCREMENTALSCANNER_H
