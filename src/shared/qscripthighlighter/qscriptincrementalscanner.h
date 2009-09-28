#ifndef QSCRIPTINCREMENTALSCANNER_H
#define QSCRIPTINCREMENTALSCANNER_H

#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>

namespace SharedTools {

class QScriptIncrementalScanner
{
public:

    struct Token {
        int offset;
        int length;
        enum Kind {
            Empty,
            Keyword,
            Type,
            Label,
            String,
            Comment,
            Number,
            LeftParenthesis,
            RightParenthesis,
            LeftBrace,
            RightBrace,
            LeftBracket,
            RightBracket,
            PreProcessor
        } kind;

        Token(int o, int l, Kind k): offset(o), length(l), kind(k) {}
    };

public:
    QScriptIncrementalScanner(bool duiEnabled = false);
    virtual ~QScriptIncrementalScanner();

    void setKeywords(const QSet<QString> &keywords)
    { m_keywords = keywords; }

    void reset();

    void operator()(int startState, const QString &text);

    int endState() const
    { return m_endState; }

    int firstNonSpace() const
    { return m_firstNonSpace; }

    QList<QScriptIncrementalScanner::Token> tokens() const
    { return m_tokens; }

private:
    void blockEnd(int state, int firstNonSpace)
    { m_endState = state; m_firstNonSpace = firstNonSpace; }
    void setFormat(int start, int count, Token::Kind kind)
    { m_tokens.append(Token(start, count, kind)); }
    void highlightKeyword(int currentPos, const QString &buffer);
    void openingParenthesis(char c, int i);
    void closingParenthesis(char c, int i);

private:
    QSet<QString> m_keywords;
    bool m_duiEnabled;
    int m_endState;
    int m_firstNonSpace;
    QList<QScriptIncrementalScanner::Token> m_tokens;
};

} // namespace SharedTools

#endif // QSCRIPTINCREMENTALSCANNER_H
