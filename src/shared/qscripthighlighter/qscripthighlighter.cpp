/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qscripthighlighter.h"

#include <QtCore/QSet>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDebug>

namespace SharedTools {

QScriptHighlighter::QScriptHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent),
      m_duiEnabled(false)
{
    qscriptKeywords.insert(QLatin1String("Infinity"));
    qscriptKeywords.insert(QLatin1String("NaN"));
    qscriptKeywords.insert(QLatin1String("abstract"));
    qscriptKeywords.insert(QLatin1String("boolean"));
    qscriptKeywords.insert(QLatin1String("break"));
    qscriptKeywords.insert(QLatin1String("byte"));
    qscriptKeywords.insert(QLatin1String("case"));
    qscriptKeywords.insert(QLatin1String("catch"));
    qscriptKeywords.insert(QLatin1String("char"));
    qscriptKeywords.insert(QLatin1String("class"));
    qscriptKeywords.insert(QLatin1String("const"));
    qscriptKeywords.insert(QLatin1String("constructor"));
    qscriptKeywords.insert(QLatin1String("continue"));
    qscriptKeywords.insert(QLatin1String("debugger"));
    qscriptKeywords.insert(QLatin1String("default"));
    qscriptKeywords.insert(QLatin1String("delete"));
    qscriptKeywords.insert(QLatin1String("do"));
    qscriptKeywords.insert(QLatin1String("double"));
    qscriptKeywords.insert(QLatin1String("else"));
    qscriptKeywords.insert(QLatin1String("enum"));
    qscriptKeywords.insert(QLatin1String("export"));
    qscriptKeywords.insert(QLatin1String("extends"));
    qscriptKeywords.insert(QLatin1String("false"));
    qscriptKeywords.insert(QLatin1String("final"));
    qscriptKeywords.insert(QLatin1String("finally"));
    qscriptKeywords.insert(QLatin1String("float"));
    qscriptKeywords.insert(QLatin1String("for"));
    qscriptKeywords.insert(QLatin1String("function"));
    qscriptKeywords.insert(QLatin1String("goto"));
    qscriptKeywords.insert(QLatin1String("if"));
    qscriptKeywords.insert(QLatin1String("implements"));
    qscriptKeywords.insert(QLatin1String("import"));
    qscriptKeywords.insert(QLatin1String("in"));
    qscriptKeywords.insert(QLatin1String("instanceof"));
    qscriptKeywords.insert(QLatin1String("int"));
    qscriptKeywords.insert(QLatin1String("interface"));
    qscriptKeywords.insert(QLatin1String("long"));
    qscriptKeywords.insert(QLatin1String("native"));
    qscriptKeywords.insert(QLatin1String("new"));
    qscriptKeywords.insert(QLatin1String("package"));
    qscriptKeywords.insert(QLatin1String("private"));
    qscriptKeywords.insert(QLatin1String("protected"));
    qscriptKeywords.insert(QLatin1String("public"));
    qscriptKeywords.insert(QLatin1String("return"));
    qscriptKeywords.insert(QLatin1String("short"));
    qscriptKeywords.insert(QLatin1String("static"));
    qscriptKeywords.insert(QLatin1String("super"));
    qscriptKeywords.insert(QLatin1String("switch"));
    qscriptKeywords.insert(QLatin1String("synchronized"));
    qscriptKeywords.insert(QLatin1String("this"));
    qscriptKeywords.insert(QLatin1String("throw"));
    qscriptKeywords.insert(QLatin1String("throws"));
    qscriptKeywords.insert(QLatin1String("transient"));
    qscriptKeywords.insert(QLatin1String("true"));
    qscriptKeywords.insert(QLatin1String("try"));
    qscriptKeywords.insert(QLatin1String("typeof"));
    qscriptKeywords.insert(QLatin1String("undefined"));
    qscriptKeywords.insert(QLatin1String("var"));
    qscriptKeywords.insert(QLatin1String("void"));
    qscriptKeywords.insert(QLatin1String("volatile"));
    qscriptKeywords.insert(QLatin1String("while"));
    qscriptKeywords.insert(QLatin1String("with"));    // end

    setFormats(defaultFormats());
}

bool QScriptHighlighter::isDuiEnabled() const
{ return m_duiEnabled; }

void QScriptHighlighter::setDuiEnabled(bool enabled)
{ m_duiEnabled = enabled; }

void QScriptHighlighter::highlightBlock(const QString &text)
{
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
    // tokens
    enum {
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

    static const uchar table[NumStates][NumInputs] = {
        { StateStandard,      StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard }, // StateStandard
        { StateStandard,      StateNumber,   StateCCommentStart2, StateCppCommentStart2, StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard }, // StateCommentStart1
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCComment,         StateCComment,   StateCComment,   StateCComment,     StateCComment,    StateCComment,     StateCComment }, // StateCCommentStart2
        { StateCppComment,    StateCppComment, StateCppComment,     StateCppComment,       StateCppComment, StateCppComment, StateCppComment,   StateCppComment,  StateCppComment,   StateCppComment }, // CppCommentStart2
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCComment,         StateCComment,   StateCComment,   StateCComment,     StateCComment,    StateCComment,     StateCComment }, // StateCComment
        { StateCppComment,    StateCppComment, StateCppComment,     StateCppComment,       StateCppComment, StateCppComment, StateCppComment,   StateCppComment,  StateCppComment,   StateCppComment }, // StateCppComment
        { StateCComment,      StateCComment,   StateCCommentEnd1,   StateCCommentEnd2,     StateCComment,   StateCComment,   StateCComment,     StateCComment,    StateCComment,     StateCComment }, // StateCCommentEnd1
        { StateStandard,      StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard }, // StateCCommentEnd2
        { StateString,        StateString,     StateString,         StateString,           StateString,     StateString,     StateString,       StateStringEnd,   StateString,       StateString }, // StateStringStart
        { StateString,        StateString,     StateString,         StateString,           StateString,     StateString,     StateString,       StateStringEnd,   StateString,       StateString }, // StateString
        { StateStandard,      StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard }, // StateStringEnd
        { StateString2,       StateString2,    StateString2,        StateString2,          StateString2,    StateString2,    StateString2,      StateString2,     StateString2End,   StateString2 }, // StateString2Start
        { StateString2,       StateString2,    StateString2,        StateString2,          StateString2,    StateString2,    StateString2,      StateString2,     StateString2End,   StateString2 }, // StateString2
        { StateStandard,      StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard }, // StateString2End
        { StateNumber,        StateNumber,     StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard }, // StateNumber
        { StatePreProcessor,  StateStandard,   StateStandard,       StateCommentStart1,    StateStandard,   StateStandard,   StatePreProcessor, StateStringStart, StateString2Start, StateStandard } // StatePreProcessor
    };

    QString buffer;
    buffer.reserve(text.length());
    QTextCharFormat emptyFormat;

    int state = onBlockStart();
    if (text.isEmpty()) {
        onBlockEnd(state, 0);
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
        const QChar c = text.at(i);

        bool lookAtBinding = false;

        if (lastWasBackSlash) {
            input = InputSep;
        } else {
            switch (c.toLatin1()) {
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
                    onOpeningParenthesis(c, i);
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
                        onClosingParenthesis(c, i);
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
                    if (alphabeth.contains(lastChar)
                        && (!mathChars.contains(lastChar) || !numbers.contains(text.at(i - 1)))) {
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

                              if (state == StateStandard && !questionMark &&
                                  lastChar != ':' && nextChar != ':') {
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
                                      setFormat(start, i - start, m_formats[KeywordFormat]);
                                  } else {
                                      setFormat(start, i - start, m_formats[LabelFormat]);
                                  }
                              }
                              break;
                          }
                default: {
                             if (!questionMark && c == QLatin1Char('?'))
                                 questionMark = true;
                             if (c.isLetter() || c == QLatin1Char('_'))
                                 input = InputAlpha;
                             else
                                 input = InputSep;
                         } break;
            }
        }

        if (input != InputSpace) {
            if (firstNonSpace < 0)
                firstNonSpace = i;
            lastNonSpace = i;
        }

        lastWasBackSlash = !lastWasBackSlash && c == QLatin1Char('\\');

        if (input == InputAlpha)
            buffer += c;

        state = table[state][input];

        switch (state) {
            case StateStandard: {
                                    setFormat(i, 1, emptyFormat);
                                    if (makeLastStandard)
                                        setFormat(i - 1, 1, emptyFormat);
                                    makeLastStandard = false;
                                    if (!buffer.isEmpty() && input != InputAlpha ) {
                                        if (! lookAtBinding)
                                            highlightKeyword(i, buffer);
                                        buffer.clear();
                                    }
                                } break;
            case StateCommentStart1:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = true;
                                buffer.resize(0);
                                break;
            case StateCCommentStart2:
                                setFormat(i - 1, 2, m_formats[CommentFormat]);
                                makeLastStandard = false;
                                buffer.resize(0);
                                break;
            case StateCppCommentStart2:
                                setFormat(i - 1, 2, m_formats[CommentFormat]);
                                makeLastStandard = false;
                                buffer.resize(0);
                                break;
            case StateCComment:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, m_formats[CommentFormat]);
                                buffer.resize(0);
                                break;
            case StateCppComment:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, m_formats[CommentFormat]);
                                buffer.resize(0);
                                break;
            case StateCCommentEnd1:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, m_formats[CommentFormat]);
                                buffer.resize(0);
                                break;
            case StateCCommentEnd2:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, m_formats[CommentFormat]);
                                buffer.resize(0);
                                break;
            case StateStringStart:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, emptyFormat);
                                buffer.resize(0);
                                break;
            case StateString:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, m_formats[StringFormat]);
                                buffer.resize(0);
                                break;
            case StateStringEnd:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, emptyFormat);
                                buffer.resize(0);
                                break;
            case StateString2Start:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, emptyFormat);
                                buffer.resize(0);
                                break;
            case StateString2:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, m_formats[StringFormat]);
                                buffer.resize(0);
                                break;
            case StateString2End:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, emptyFormat);
                                buffer.resize(0);
                                break;
            case StateNumber:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat( i, 1, m_formats[NumberFormat]);
                                buffer.resize(0);
                                break;
            case StatePreProcessor:
                                if (makeLastStandard)
                                    setFormat(i - 1, 1, emptyFormat);
                                makeLastStandard = false;
                                setFormat(i, 1, m_formats[PreProcessorFormat]);
                                buffer.resize(0);
                                break;
        }

        lastChar = c;
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

    onBlockEnd(state, firstNonSpace);
}

void QScriptHighlighter::highlightKeyword(int currentPos, const QString &buffer)
{
    if (buffer.isEmpty())
        return;

    if ((m_duiEnabled && buffer.at(0).isUpper()) || (! m_duiEnabled && buffer.at(0) == QLatin1Char('Q'))) {
        setFormat(currentPos - buffer.length(), buffer.length(), m_formats[TypeFormat]);
    } else {
        if (qscriptKeywords.contains(buffer))
            setFormat(currentPos - buffer.length(), buffer.length(), m_formats[KeywordFormat]);
    }
}

const QVector<QTextCharFormat> &QScriptHighlighter::defaultFormats()
{
    static QVector<QTextCharFormat> rc;
    if (rc.empty()) {
        rc.resize(NumFormats);
        rc[NumberFormat].setForeground(Qt::blue);
        rc[StringFormat].setForeground(Qt::darkGreen);
        rc[TypeFormat].setForeground(Qt::darkMagenta);
        rc[KeywordFormat].setForeground(Qt::darkYellow);
        rc[LabelFormat].setForeground(Qt::darkRed);
        rc[CommentFormat].setForeground(Qt::red);
        rc[CommentFormat].setFontItalic(true);
        rc[PreProcessorFormat].setForeground(Qt::darkBlue);
    }
    return rc;
}

void QScriptHighlighter::setFormats(const QVector<QTextCharFormat> &s)
{
    qCopy(s.constBegin(), s.constEnd(), m_formats);
}

int QScriptHighlighter::onBlockStart()
{
    int state = 0;
    int previousState = previousBlockState();
    if (previousState != -1)
        state = previousState;
    return state;
}
void QScriptHighlighter::onOpeningParenthesis(QChar, int) {}
void QScriptHighlighter::onClosingParenthesis(QChar, int) {}
void QScriptHighlighter::onBlockEnd(int state, int) { return setCurrentBlockState(state); }

} // namespace SharedTools

