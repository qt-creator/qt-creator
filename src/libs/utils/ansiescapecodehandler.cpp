// Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ansiescapecodehandler.h"

#include "qtcassert.h"

#include <QPlainTextEdit>

#include <utility>

namespace Utils {

static const QString s_escape        = "\x1b[";
static const QChar s_semicolon       = ';';
static const QChar s_colorTerminator = 'm';

/*!
    \class Utils::AnsiEscapeCodeHandler
    \inmodule QtCreator

    \brief The AnsiEscapeCodeHandler class parses text and extracts ANSI escape codes from it.

    In order to preserve color information across text segments, an instance of this class
    must be stored for the lifetime of a stream.
    Also, one instance of this class should not handle multiple streams (at least not
    at the same time).

    Its main function is parseText(), which accepts text and default QTextCharFormat.
    This function is designed to parse text and split colored text to smaller strings,
    with their appropriate formatting information set inside QTextCharFormat.

    Usage:
    \list
    \li Create new instance of AnsiEscapeCodeHandler for a stream.
    \li To add new text, call parseText() with the text and a default QTextCharFormat.
        The result of this function is a list of strings with formats set in appropriate
        QTextCharFormat.
    \endlist
*/

static QColor ansiColor(uint code)
{
    QTC_ASSERT(code < 8, return QColor());

    const int red   = code & 1 ? 170 : 0;
    const int green = code & 2 ? 170 : 0;
    const int blue  = code & 4 ? 170 : 0;
    return QColor(red, green, blue);
}

QList<FormattedText> AnsiEscapeCodeHandler::parseText(const FormattedText &input)
{
    enum AnsiEscapeCodes {
        ResetFormat            =  0,
        BoldText               =  1,
        TextColorStart         = 30,
        TextColorEnd           = 37,
        RgbTextColor           = 38,
        DefaultTextColor       = 39,
        BackgroundColorStart   = 40,
        BackgroundColorEnd     = 47,
        RgbBackgroundColor     = 48,
        DefaultBackgroundColor = 49
    };

    const QChar eraseToEol      = 'K';

    QList<FormattedText> outputData;
    QTextCharFormat charFormat = m_previousFormatClosed ? input.format : m_previousFormat;
    QString strippedText;
    if (m_pendingText.isEmpty()) {
        strippedText = input.text;
    } else {
        strippedText = m_pendingText.append(input.text);
        m_pendingText.clear();
    }

    while (!strippedText.isEmpty()) {
        QTC_ASSERT(m_pendingText.isEmpty(), break);
        if (m_waitingForTerminator) {
            // We ignore all escape codes taking string arguments.
            QString terminator = "\x1b\\";
            int terminatorPos = strippedText.indexOf(terminator);
            if (terminatorPos == -1 && !m_alternateTerminator.isEmpty()) {
                terminator = m_alternateTerminator;
                terminatorPos = strippedText.indexOf(terminator);
            }
            if (terminatorPos == -1) {
                m_pendingText = strippedText;
                break;
            }
            m_waitingForTerminator = false;
            m_alternateTerminator.clear();
            strippedText.remove(0, terminatorPos + terminator.size());
            if (strippedText.isEmpty())
                break;
        }
        const int escapePos = strippedText.indexOf(s_escape.at(0));
        if (escapePos < 0) {
            outputData << FormattedText(strippedText, charFormat);
            break;
        } else if (escapePos != 0) {
            outputData << FormattedText(strippedText.left(escapePos), charFormat);
            strippedText.remove(0, escapePos);
        }
        QTC_ASSERT(strippedText.at(0) == s_escape.at(0), break);

        while (!strippedText.isEmpty() && s_escape.at(0) == strippedText.at(0)) {
            if (s_escape.startsWith(strippedText)) {
                // control secquence is not complete
                m_pendingText += strippedText;
                strippedText.clear();
                break;
            }
            if (!strippedText.startsWith(s_escape)) {
                switch (strippedText.at(1).toLatin1()) {
                case '\\': // Unexpected terminator sequence.
                    QTC_CHECK(false);
                    Q_FALLTHROUGH();
                case 'N': case 'O': // Ignore unsupported single-character sequences.
                    strippedText.remove(0, 2);
                    break;
                case ']':
                    m_alternateTerminator = QChar(7);
                    Q_FALLTHROUGH();
                case 'P':  case 'X': case '^': case '_':
                    strippedText.remove(0, 2);
                    m_waitingForTerminator = true;
                    break;
                default:
                    // not a control sequence
                    m_pendingText.clear();
                    outputData << FormattedText(strippedText.left(1), charFormat);
                    strippedText.remove(0, 1);
                    continue;
                }
                break;
            }
            m_pendingText += strippedText.mid(0, s_escape.size());
            strippedText.remove(0, s_escape.size());

            // \e[K is not supported. Just strip it.
            if (strippedText.startsWith(eraseToEol)) {
                m_pendingText.clear();
                strippedText.remove(0, 1);
                continue;
            }
            // get the number
            QString strNumber;
            QStringList numbers;
            while (!strippedText.isEmpty()) {
                if (strippedText.at(0).isDigit()) {
                    strNumber += strippedText.at(0);
                } else {
                    if (!strNumber.isEmpty())
                        numbers << strNumber;
                    if (strNumber.isEmpty() || strippedText.at(0) != s_semicolon)
                        break;
                    strNumber.clear();
                }
                m_pendingText += strippedText.mid(0, 1);
                strippedText.remove(0, 1);
            }
            if (strippedText.isEmpty())
                break;

            // remove terminating char
            if (!strippedText.startsWith(s_colorTerminator)) {
                m_pendingText.clear();
                strippedText.remove(0, 1);
                break;
            }
            // got consistent control sequence, ok to clear pending text
            m_pendingText.clear();
            strippedText.remove(0, 1);

            if (numbers.isEmpty()) {
                charFormat = input.format;
                endFormatScope();
            }

            for (int i = 0; i < numbers.size(); ++i) {
                const uint code = numbers.at(i).toUInt();

                if (code >= TextColorStart && code <= TextColorEnd) {
                    charFormat.setForeground(ansiColor(code - TextColorStart));
                    setFormatScope(charFormat);
                } else if (code >= BackgroundColorStart && code <= BackgroundColorEnd) {
                    charFormat.setBackground(ansiColor(code - BackgroundColorStart));
                    setFormatScope(charFormat);
                } else {
                    switch (code) {
                    case ResetFormat:
                        charFormat = input.format;
                        endFormatScope();
                        break;
                    case BoldText:
                        charFormat.setFontWeight(QFont::Bold);
                        setFormatScope(charFormat);
                        break;
                    case DefaultTextColor:
                        charFormat.setForeground(input.format.foreground());
                        setFormatScope(charFormat);
                        break;
                    case DefaultBackgroundColor:
                        charFormat.setBackground(input.format.background());
                        setFormatScope(charFormat);
                        break;
                    case RgbTextColor:
                    case RgbBackgroundColor:
                        // See http://en.wikipedia.org/wiki/ANSI_escape_code#Colors
                        if (++i >= numbers.size())
                            break;
                        switch (numbers.at(i).toInt()) {
                        case 2:
                            // RGB set with format: 38;2;<r>;<g>;<b>
                            if ((i + 3) < numbers.size()) {
                                (code == RgbTextColor) ?
                                      charFormat.setForeground(QColor(numbers.at(i + 1).toInt(),
                                                                      numbers.at(i + 2).toInt(),
                                                                      numbers.at(i + 3).toInt())) :
                                      charFormat.setBackground(QColor(numbers.at(i + 1).toInt(),
                                                                      numbers.at(i + 2).toInt(),
                                                                      numbers.at(i + 3).toInt()));
                                setFormatScope(charFormat);
                            }
                            i += 3;
                            break;
                        case 5:
                            // 256 color mode with format: 38;5;<i>
                            uint index = numbers.at(i + 1).toUInt();

                            QColor color;
                            if (index < 8) {
                                // The first 8 colors are standard low-intensity ANSI colors.
                                color = ansiColor(index);
                            } else if (index < 16) {
                                // The next 8 colors are standard high-intensity ANSI colors.
                                color = ansiColor(index - 8).lighter(150);
                            } else if (index < 232) {
                                // The next 216 colors are a 6x6x6 RGB cube.
                                uint o = index - 16;
                                color = QColor((o / 36) * 51, ((o / 6) % 6) * 51, (o % 6) * 51);
                            } else {
                                // The last 24 colors are a greyscale gradient.
                                int grey = int((index - 232) * 11);
                                color = QColor(grey, grey, grey);
                            }

                            if (code == RgbTextColor)
                                charFormat.setForeground(color);
                            else
                                charFormat.setBackground(color);

                            setFormatScope(charFormat);
                            ++i;
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    return outputData;
}

void AnsiEscapeCodeHandler::endFormatScope()
{
    m_previousFormatClosed = true;
}

void AnsiEscapeCodeHandler::setTextInDocument(QTextDocument *doc, const QString &text)
{
    AnsiEscapeCodeHandler handler;
    const QList<FormattedText> formattedTextList = handler.parseText(FormattedText(text));
    doc->clear();
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (const auto &formattedChunk : formattedTextList)
        cursor.insertText(formattedChunk.text, formattedChunk.format);
    cursor.endEditBlock();
    doc->setModified(false);
}

QString AnsiEscapeCodeHandler::ansiFromColor(const QColor &color)
{
    // RGB color is ESC[38;2;<r>;<g>;<b>m
    // https://en.wikipedia.org/wiki/ANSI_escape_code#24-bit
    return QStringLiteral("%1;2;%2;%3;%4m")
        .arg(s_escape + "38")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue());
}

QString AnsiEscapeCodeHandler::noColor()
{
    return s_escape + s_colorTerminator;
}

void AnsiEscapeCodeHandler::setFormatScope(const QTextCharFormat &charFormat)
{
    m_previousFormat = charFormat;
    m_previousFormatClosed = false;
}

// Number of bytes making up the UTF-8 character starting with \a c. A
// continuation or otherwise invalid byte counts as one, so that non-UTF-8 input
// passes through unchanged instead of being replaced.
static int utf8SequenceLength(uchar c)
{
    if (c < 0x80)
        return 1;
    if ((c & 0xE0) == 0xC0)
        return 2;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xF8) == 0xF0)
        return 4;
    return 1;
}

// Length of the escape sequence starting at \a pos, which holds an ESC. An
// unterminated sequence consumes the rest of the input: none of it is
// displayable text.
static int escapeSequenceLength(const QByteArray &data, int pos)
{
    const int size = data.size();
    const int rest = size - pos;
    if (rest < 2)
        return rest;
    const char introducer = data.at(pos + 1);
    int i = pos + 2;
    switch (introducer) {
    case '[':
        // CSI: parameter bytes, intermediate bytes, then one final byte.
        for (; i < size && uchar(data.at(i)) >= 0x30 && uchar(data.at(i)) <= 0x3F; ++i)
            ;
        for (; i < size && uchar(data.at(i)) >= 0x20 && uchar(data.at(i)) <= 0x2F; ++i)
            ;
        if (i < size && uchar(data.at(i)) >= 0x40 && uchar(data.at(i)) <= 0x7E)
            return i + 1 - pos;
        return rest;
    case ']': case 'P': case 'X': case '^': case '_':
        // OSC, DCS, SOS, PM, APC: a string terminated by BEL or ST.
        for (; i < size; ++i) {
            if (data.at(i) == '\a')
                return i + 1 - pos;
            if (data.at(i) == '\x1b' && i + 1 < size && data.at(i + 1) == '\\')
                return i + 2 - pos;
        }
        return rest;
    case '(': case ')': case '*': case '+': case '#':
        // Character set selection: one designator byte.
        return i < size ? 3 : rest;
    default:
        return 2;
    }
}

/*!
    Reduces terminal output \a data to the plain text a terminal would display,
    with line endings normalized to \a lineEnding, so that callers can match on
    it byte-exactly.

    A pseudo-terminal is not a transparent byte pipe: the remote line editor
    (and ConPTY on Windows) may inject escape sequences and redraw lines using
    carriage returns or backspaces. Escape sequences are removed here, and
    carriage returns, backspaces and bells are applied to the current line.

    \a data must not end in the middle of a line, because an escape sequence cut
    in half cannot be recognized. Buffer up to the last newline and pass that.
*/
QByteArray normalizeTerminalOutput(const QByteArray &data, TerminalLineEnding lineEnding)
{
    const QByteArray newLine = lineEnding == TerminalLineEnding::CrLf ? "\r\n" : "\n";
    const int size = data.size();
    QByteArray result;
    result.reserve(size);
    QList<QByteArray> line; // One entry per displayed character, so that a
    int column = 0;         // backspace does not cut a UTF-8 character in half.

    for (int i = 0; i < size; ) {
        const char c = data.at(i);
        if (c == '\x1b') {
            i += escapeSequenceLength(data, i);
        } else if (c == '\n') {
            for (const QByteArray &cell : std::as_const(line))
                result += cell;
            result += newLine;
            line.clear();
            column = 0;
            ++i;
        } else if (c == '\r') {
            column = 0; // The line is being redrawn from the first column.
            ++i;
        } else if (c == '\b') {
            if (column > 0)
                --column;
            ++i;
        } else if (c == '\a') {
            ++i;
        } else {
            const int length = qMin(utf8SequenceLength(uchar(c)), size - i);
            if (column < line.size())
                line[column] = data.mid(i, length);
            else
                line.append(data.mid(i, length));
            ++column;
            i += length;
        }
    }
    for (const QByteArray &cell : std::as_const(line))
        result += cell;
    return result;
}

} // namespace Utils
