/****************************************************************************
**
** Copyright (C) 2014 Petar Perisin <petar.perisin@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "ansiescapecodehandler.h"
#include <utils/qtcassert.h>

namespace Utils {

/*!
    \class Utils::AnsiEscapeCodeHandler

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

AnsiEscapeCodeHandler::AnsiEscapeCodeHandler() :
    m_previousFormatClosed(true)
{
}

static QColor ansiColor(uint code)
{
    QTC_ASSERT(code < 8, return QColor());

    const int red   = code & 1 ? 170 : 0;
    const int green = code & 2 ? 170 : 0;
    const int blue  = code & 4 ? 170 : 0;
    return QColor(red, green, blue);
}

QList<StringFormatPair> AnsiEscapeCodeHandler::parseText(const QString &text,
                                                         const QTextCharFormat &defaultFormat)
{
    QList<StringFormatPair> outputData;

    QTextCharFormat charFormat = m_previousFormatClosed ? defaultFormat : m_previousFormat;

    const QString escape = QLatin1String("\x1b[");
    if (!text.contains(escape)) {
        outputData << StringFormatPair(text, charFormat);
        return outputData;
    } else if (!text.startsWith(escape)) {
        outputData << StringFormatPair(text.left(text.indexOf(escape)), charFormat);
    }

    const QChar semicolon       = QLatin1Char(';');
    const QChar colorTerminator = QLatin1Char('m');
    // strippedText always starts with "\e["
    QString strippedText = text.mid(text.indexOf(escape));
    while (!strippedText.isEmpty()) {
        while (strippedText.startsWith(escape)) {
            strippedText.remove(0, 2);

            // get the number
            QString strNumber;
            QStringList numbers;
            while (strippedText.at(0).isDigit() || strippedText.at(0) == semicolon) {
                if (strippedText.at(0).isDigit()) {
                    strNumber += strippedText.at(0);
                } else {
                    numbers << strNumber;
                    strNumber.clear();
                }
                strippedText.remove(0, 1);
            }
            if (!strNumber.isEmpty())
                numbers << strNumber;

            // remove terminating char
            if (!strippedText.startsWith(colorTerminator)) {
                strippedText.remove(0, 1);
                continue;
            }
            strippedText.remove(0, 1);

            if (numbers.isEmpty()) {
                charFormat = defaultFormat;
                endFormatScope();
            }

            for (int i = 0; i < numbers.size(); ++i) {
                const int code = numbers.at(i).toInt();

                if (code >= TextColorStart && code <= TextColorEnd) {
                    charFormat.setForeground(ansiColor(code - TextColorStart));
                    setFormatScope(charFormat);
                } else if (code >= BackgroundColorStart && code <= BackgroundColorEnd) {
                    charFormat.setBackground(ansiColor(code - BackgroundColorStart));
                    setFormatScope(charFormat);
                } else {
                    switch (code) {
                    case ResetFormat:
                        charFormat = defaultFormat;
                        endFormatScope();
                        break;
                    case BoldText:
                        charFormat.setFontWeight(QFont::Bold);
                        setFormatScope(charFormat);
                        break;
                    case DefaultTextColor:
                        charFormat.setForeground(defaultFormat.foreground());
                        setFormatScope(charFormat);
                        break;
                    case DefaultBackgroundColor:
                        charFormat.setBackground(defaultFormat.background());
                        setFormatScope(charFormat);
                        break;
                    case RgbTextColor:
                    case RgbBackgroundColor:
                        if (++i >= numbers.size())
                            break;
                        if (numbers.at(i).toInt() == 2) {
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
                        } else if (numbers.at(i).toInt() == 5) {
                            // rgb set with format: 38;5;<i>
                            // unsupported because of unclear documentation, so we just skip <i>
                            ++i;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        if (strippedText.isEmpty())
            break;
        int index = strippedText.indexOf(escape);
        if (index > 0) {
            outputData << StringFormatPair(strippedText.left(index), charFormat);
            strippedText.remove(0, index);
        } else if (index == -1) {
            outputData << StringFormatPair(strippedText, charFormat);
            break;
        }
    }
    return outputData;
}

void AnsiEscapeCodeHandler::endFormatScope()
{
    m_previousFormatClosed = true;
}

void AnsiEscapeCodeHandler::setFormatScope(const QTextCharFormat &charFormat)
{
    m_previousFormat = charFormat;
    m_previousFormatClosed = false;
}

} // namespace Utils
