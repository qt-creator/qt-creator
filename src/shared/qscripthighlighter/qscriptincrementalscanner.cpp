#include "qscriptincrementalscanner.h"

#include <QTextCharFormat>

using namespace SharedTools;

QScriptIncrementalScanner::QScriptIncrementalScanner(bool duiEnabled):
        m_duiEnabled(duiEnabled)
{
    reset();
}

QScriptIncrementalScanner::~QScriptIncrementalScanner()
{}

void QScriptIncrementalScanner::reset()
{
    m_endState = -1;
    m_firstNonSpace = -1;
    m_tokens.clear();
}

void QScriptIncrementalScanner::operator()(int startState, const QString &text)
{
    reset();

    // tokens
    enum TokenKind {
        InputAlpha,
        InputNumber,
        InputAsterix,
        InputSlash,
        InputParen,
        InputSpace,
        InputHash,
        InputQuotation,
        InputApostrophe,
        InputSep,
        NumInputs
    };

    // states
    enum {
        StateStandard,
        StateCommentStart1,
        StateCCommentStart2,
        StateCppCommentStart2,
        StateCComment,
        StateCppComment,
        StateCCommentEnd1,
        StateCCommentEnd2,
        StateStringStart,
        StateString,
        StateStringEnd,
        StateString2Start,
        StateString2,
        StateString2End,
        StateNumber,
        StatePreProcessor,
        NumStates
    };

    static const uchar table[NumStates][NumInputs] = {
        { StateStandard,      StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard   }, // StateStandard
        { StateStandard,      StateNumber,     StateCCommentStart2, StateCppCommentStart2, StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard   }, // StateCommentStart1
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCComment,         StateCComment,   StateCComment,   StateCComment,     StateCComment,    StateCComment,     StateCComment   }, // StateCCommentStart2
        { StateCppComment,    StateCppComment, StateCppComment,     StateCppComment,       StateCppComment, StateCppComment, StateCppComment,   StateCppComment,  StateCppComment,   StateCppComment }, // CppCommentStart2
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCComment,         StateCComment,   StateCComment,   StateCComment,     StateCComment,    StateCComment,     StateCComment   }, // StateCComment
        { StateCppComment,    StateCppComment, StateCppComment,     StateCppComment,       StateCppComment, StateCppComment, StateCppComment,   StateCppComment,  StateCppComment,   StateCppComment }, // StateCppComment
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCCommentEnd2,     StateCComment,   StateCComment,   StateCComment,     StateCComment,    StateCComment,     StateCComment   }, // StateCCommentEnd1
        { StateStandard,      StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard   }, // StateCCommentEnd2
        { StateString,        StateString,     StateString,         StateString,           StateString,     StateString,     StateString,       StateStringEnd,   StateString,       StateString     }, // StateStringStart
        { StateString,        StateString,     StateString,         StateString,           StateString,     StateString,     StateString,       StateStringEnd,   StateString,       StateString     }, // StateString
        { StateStandard,      StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard   }, // StateStringEnd
        { StateString2,       StateString2,    StateString2,        StateString2,          StateString2,    StateString2,    StateString2,      StateString2,     StateString2End,   StateString2    }, // StateString2Start
        { StateString2,       StateString2,    StateString2,        StateString2,          StateString2,    StateString2,    StateString2,      StateString2,     StateString2End,   StateString2    }, // StateString2
        { StateStandard,      StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard   }, // StateString2End
        { StateNumber,        StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard   }, // StateNumber
        { StatePreProcessor,  StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard   }  // StatePreProcessor
    };

    QString buffer;
    buffer.reserve(text.length());

    int state = startState;
    if (text.isEmpty()) {
        blockEnd(state, 0);
        return;
    }

    int input = -1;
    int i = 0;
    bool lastWasBackSlash = false;
    bool makeLastStandard = false;

    static const QString alphabeth = QLatin1String("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static const QString mathChars = QString::fromLatin1("xXeE");
    static const QString numbers = QString::fromLatin1("0123456789");
    bool questionMark = false;
    QChar lastChar;

    int firstNonSpace = -1;
    int lastNonSpace = -1;

    forever {
        const QChar qc = text.at(i);

        bool lookAtBinding = false;

        if (lastWasBackSlash) {
            input = InputSep;
        } else {
            const char c = qc.toLatin1();
            switch (c) {
                case '*':
                    input = InputAsterix;
                    break;
                case '/':
                    input = InputSlash;
                    break;
                case '(': case '[': case '{':
                    input = InputParen;
                    if (state == StateStandard
                        || state == StateNumber
                        || state == StatePreProcessor
                        || state == StateCCommentEnd2
                        || state == StateCCommentEnd1
                        || state == StateString2End
                        || state == StateStringEnd
                       )
                    openingParenthesis(c, i);
                    break;
                case ')': case ']': case '}':
                    input = InputParen;
                    if (state == StateStandard
                        || state == StateNumber
                        || state == StatePreProcessor
                        || state == StateCCommentEnd2
                        || state == StateCCommentEnd1
                        || state == StateString2End
                        || state == StateStringEnd
                       ) {
                        closingParenthesis(c, i);
                    }
                    break;
                case '#':
                    input = InputHash;
                    break;
                case '"':
                    input = InputQuotation;
                    break;
                case '\'':
                    input = InputApostrophe;
                    break;
                case ' ':
                    input = InputSpace;
                    break;
                case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9': case '0':
                    if (alphabeth.contains(lastChar) && (!mathChars.contains(lastChar) || !numbers.contains(text.at(i - 1)))) {
                        input = InputAlpha;
                    } else {
                        if (input == InputAlpha && numbers.contains(lastChar))
                            input = InputAlpha;
                        else
                            input = InputNumber;
                    }
                    break;
                case ':': {
                    input = InputSep;
                    QChar nextChar = ' ';
                    if (i < text.length() - 1)
                        nextChar = text.at(i + 1);

                    if (state == StateStandard && !questionMark && lastChar != ':' && nextChar != ':') {
                        int start = i - 1;

                        // skip white spaces
                        for (; start != -1; --start) {
                            if (! text.at(start).isSpace())
                                break;
                        }

                        int lastNonSpace = start + 1;

                        for (; start != -1; --start) {
                            const QChar ch = text.at(start);
                            if (! (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('.')))
                                break;
                        }

                        ++start;

                        lookAtBinding = true;

                        if (m_duiEnabled && text.midRef(start, lastNonSpace - start) == QLatin1String("id")) {
                            setFormat(start, i - start, Token::Keyword);
                        } else {
                            setFormat(start, i - start, Token::Label);
                        }
                    }
                    break;
                }
                default: {
                    if (!questionMark && qc == QLatin1Char('?'))
                        questionMark = true;
                    if (qc.isLetter() || qc == QLatin1Char('_'))
                        input = InputAlpha;
                    else
                        input = InputSep;
                    break;
                }
            }
        }

        if (input != InputSpace) {
            if (firstNonSpace < 0)
                firstNonSpace = i;
            lastNonSpace = i;
        }

        lastWasBackSlash = !lastWasBackSlash && qc == QLatin1Char('\\');

        if (input == InputAlpha)
            buffer += qc;

        state = table[state][input];

        switch (state) {
            case StateStandard: {
                setFormat(i, 1, Token::Empty);
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                if (!buffer.isEmpty() && input != InputAlpha ) {
                    if (! lookAtBinding)
                        highlightKeyword(i, buffer);
                    buffer.clear();
                }

                break;
            }

            case StateCommentStart1:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = true;
                buffer.resize(0);
                break;
            case StateCCommentStart2:
                setFormat(i - 1, 2, Token::Comment);
                makeLastStandard = false;
                buffer.resize(0);
                break;
            case StateCppCommentStart2:
                setFormat(i - 1, 2, Token::Comment);
                makeLastStandard = false;
                buffer.resize(0);
                break;
            case StateCComment:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Comment);
                buffer.resize(0);
                break;
            case StateCppComment:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Comment);
                buffer.resize(0);
                break;
            case StateCCommentEnd1:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Comment);
                buffer.resize(0);
                break;
            case StateCCommentEnd2:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Comment);
                buffer.resize(0);
                break;
            case StateStringStart:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Empty);
                buffer.resize(0);
                break;
            case StateString:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::String);
                buffer.resize(0);
                break;
            case StateStringEnd:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Empty);
                buffer.resize(0);
                break;
            case StateString2Start:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Empty);
                buffer.resize(0);
                break;
            case StateString2:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::String);
                buffer.resize(0);
                break;
            case StateString2End:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::Empty);
                buffer.resize(0);
                break;
            case StateNumber:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat( i, 1, Token::Number);
                buffer.resize(0);
                break;
            case StatePreProcessor:
                if (makeLastStandard)
                    setFormat(i - 1, 1, Token::Empty);
                makeLastStandard = false;
                setFormat(i, 1, Token::PreProcessor);
                buffer.resize(0);
                break;
        }

        lastChar = qc;
        i++;
        if (i >= text.length())
            break;
    }

    highlightKeyword(text.length(), buffer);

    if (state == StateCComment
        || state == StateCCommentEnd1
        || state == StateCCommentStart2
       ) {
        state = StateCComment;
    } else {
        state = StateStandard;
    }

    blockEnd(state, firstNonSpace);
}

void QScriptIncrementalScanner::highlightKeyword(int currentPos, const QString &buffer)
{
    if (buffer.isEmpty())
        return;

    if ((m_duiEnabled && buffer.at(0).isUpper()) || (! m_duiEnabled && buffer.at(0) == QLatin1Char('Q'))) {
        setFormat(currentPos - buffer.length(), buffer.length(), Token::Type);
    } else {
        if (m_keywords.contains(buffer))
            setFormat(currentPos - buffer.length(), buffer.length(), Token::Keyword);
    }
}

void QScriptIncrementalScanner::openingParenthesis(char c, int i)
{
    Token::Kind kind;

    switch (c) {
        case '(':
            kind = Token::LeftParenthesis;
            break;

        case '[':
            kind = Token::LeftBracket;
            break;

        case '{':
            kind = Token::LeftBrace;
            break;

        default:
            return;
    }

    m_tokens.append(Token(i, 1, kind));
}

void QScriptIncrementalScanner::closingParenthesis(char c, int i)
{
    Token::Kind kind;

    switch (c) {
        case ')':
            kind = Token::RightParenthesis;
            break;

        case ']':
            kind = Token::RightBracket;
            break;

        case '}':
            kind = Token::RightBrace;
            break;

        default:
            return;
    }

    m_tokens.append(Token(i, 1, kind));
}
