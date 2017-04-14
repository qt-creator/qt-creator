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

#include "colorpreviewhoverhandler.h"
#include "texteditor.h"

#include <coreplugin/icore.h>
#include <utils/tooltip/tooltip.h>
#include <utils/qtcassert.h>

#include <QPoint>
#include <QColor>
#include <QTextBlock>

using namespace Core;

namespace TextEditor {

/*
 * Attempts to find a color string such as "#112233" from the word at
 * the given position in the string. Also looks for "Qt::" for recognizing
 * Qt::GlobalColor types (although there cannot be any spaces such as
 * "Qt:: yellow")
 */
static QString extractColorString(const QString &s, int pos)
{
    if (s.length() < 3 || pos < 0 || pos >= s.length())
        return QString();

    int firstPos = pos;
    do {
        QChar c = s[firstPos];
        if (c == QLatin1Char('#'))
            break;

        if (c == QLatin1Char(':')
                && (firstPos > 3)
                && (s.mid(firstPos-3, 4) == QLatin1String("Qt::"))) {
            firstPos -= 3;
            break;
        }

        if (!c.isLetterOrNumber())
            return QString();

        --firstPos;
    } while (firstPos >= 0);

    if (firstPos < 0)
        return QString();

    int lastPos = firstPos + 1;
    do {
        QChar c = s[lastPos];
        if (!(c.isLetterOrNumber() || c == QLatin1Char(':')))
            break;
        lastPos++;
    } while (lastPos < s.length());

    return s.mid(firstPos, lastPos - firstPos);
}

static QColor fromEnumString(const QString &s)
{
    const struct EnumColorMap {
        QLatin1String name;
        QColor color;
    } table[] = {
        {QLatin1String("white") , QColor(Qt::white)},
        {QLatin1String("black"), QColor(Qt::black)},
        {QLatin1String("red"), QColor(Qt::red)},
        {QLatin1String("darkRed"), QColor(Qt::darkRed)},
        {QLatin1String("green"), QColor(Qt::green)},
        {QLatin1String("darkGreen"), QColor(Qt::darkGreen)},
        {QLatin1String("blue"), QColor(Qt::blue)},
        {QLatin1String("darkBlue"), QColor(Qt::darkBlue)},
        {QLatin1String("cyan"), QColor(Qt::cyan)},
        {QLatin1String("darkCyan"), QColor(Qt::darkCyan)},
        {QLatin1String("magenta"), QColor(Qt::magenta)},
        {QLatin1String("darkMagenta"), QColor(Qt::darkMagenta)},
        {QLatin1String("yellow"), QColor(Qt::yellow)},
        {QLatin1String("darkYellow"), QColor(Qt::darkYellow)},
        {QLatin1String("gray"), QColor(Qt::gray)},
        {QLatin1String("darkGray"), QColor(Qt::darkGray)},
        {QLatin1String("lightGray"), QColor(Qt::lightGray)},
        {QLatin1String("transparent"), QColor(Qt::transparent)}
    };

    for (uint ii = 0; ii < sizeof(table) / sizeof(table[0]); ++ii) {
        if (s == table[ii].name)
            return table[ii].color;
    }

    return QColor();
}

static QColor checkColorText(const QString &str)
{
    if (str.startsWith(QLatin1Char('#')))
        return QColor(str);

    if (str.startsWith(QLatin1String("Qt::"))) {
        QString colorStr = str;
        colorStr.remove(0, 4);
        return fromEnumString(colorStr);
    }

    return QColor();
}

// looks backwards through a string for the opening brace of a function
static int findOpeningBrace(const QString &s, int startIndex)
{
    QTC_ASSERT(startIndex >= 0 && startIndex <= s.length(), return -1);

    int index = startIndex == s.length() ? startIndex - 1 : startIndex;
    while (index > 0) {
        const QChar c = s[index];
        if (c == QLatin1Char('(') || c == QLatin1Char('{'))
            return index;

        --index;
    }

    return index;
}

static int findClosingBrace(const QString &s, int startIndex)
{
    QTC_ASSERT(startIndex >= 0 && startIndex <= s.length(), return -1);

    int index = startIndex == s.length() ? startIndex - 1 : startIndex;
    const int len = s.length();
    while (index < len) {
        const QChar c = s[index];
        if (c == QLatin1Char(')') || c == QLatin1Char('}'))
            return index;

        ++index;
    }

    return -1;
}

// returns the index of the first character of the func, or negative if not valid
static int findFuncStart(const QString &s, int startIndex)
{
    QTC_ASSERT(startIndex >= 0 && startIndex <= s.length(), return -1);

    int index = startIndex == s.length() ? startIndex - 1 : startIndex;
    while (index >= 0) {
        const QChar c = s[index];
        if (!c.isLetterOrNumber()) {
            if (index == startIndex)
                return -1;

            return qMin(index + 1, startIndex);
        }
        --index;
    }

    return index;
}

static QString removeWhitespace(const QString &s)
{
    QString ret;
    ret.reserve(s.size());
    for (int ii = 0; ii < s.length(); ++ii) {
        const QChar c = s[ii];
        if (!c.isSpace())
            ret += c;

    }
    return ret;
}

/*
 * Parses the string looking for a function and its arguments.
 * The starting position is assumed to be within the braces.
 */
static bool extractFuncAndArgs(const QString &s,
                               int pos,
                               QString &retFuncName,
                               QStringList &retArgs)
{
    int openBrace = findOpeningBrace(s, pos);
    if (openBrace <= 0)
        return false;

    int closeBrace = findClosingBrace(s, openBrace + 1);
    if (closeBrace < 0)
        return false;

    int funcEnd = openBrace - 1;
    int funcStart = findFuncStart(s, funcEnd);

    if (funcStart < 0 || funcEnd <= funcStart)
        return false;

    retFuncName = removeWhitespace(s.mid(funcStart, funcEnd - funcStart + 1));

    QString argStr = s.mid(openBrace + 1, closeBrace - openBrace - 1);
    retArgs = argStr.split(QLatin1Char(','), QString::KeepEmptyParts);

    return true;
}

static QColor::Spec specForFunc(const QString &func)
{
    if ((func == QLatin1String("QColor"))
            || (func == QLatin1String("QRgb"))
            || (func == QLatin1String("rgb"))
            || func.startsWith(QLatin1String("setRgb"))
            || func.startsWith(QLatin1String("setRgba"))){
        return QColor::Rgb;
    }

    if (func.startsWith(QLatin1String("setCmyk")))
        return QColor::Cmyk;

    if (func.startsWith(QLatin1String("setHsv")))
        return QColor::Hsv;

    if (func.startsWith(QLatin1String("setHsl")))
        return QColor::Hsv;

    return QColor::Invalid;
}

static QColor colorFromArgs(const QStringList &args, QColor::Spec spec)
{
    const int maxArgs = 5;
    int vals[maxArgs];
    vals[3] = 0xff;
    vals[4] = 0xff;

    bool allOk = true;
    for (int ii = 0; ii < qMin(args.size(), maxArgs); ++ii) {
         bool ok;
         vals[ii] = args[ii].toInt(&ok, 0);
         allOk &= ok;
    }

    if (!allOk)
        return QColor();

    QColor c;
    switch (spec) {
    case QColor::Rgb:
        c.setRgb(vals[0], vals[1], vals[2], vals[3]);
        break;
    case QColor::Cmyk:
        c.setCmyk(vals[0], vals[1], vals[2], vals[3], vals[4]);
        break;
    case QColor::Hsv:
        c.setHsv(vals[0], vals[1], vals[2], vals[3]);
        break;
    case QColor::Hsl:
        c.setHsl(vals[0], vals[1], vals[2], vals[3]);
        break;
    default:
        break;
    }
    return c;
}

static QColor colorFromArgsF(const QStringList &args, QColor::Spec spec)
{
    const int maxArgs = 5;
    qreal vals[maxArgs];
    vals[3] = 1.0;
    vals[4] = 1.0;

    bool allOk = true;
    for (int ii = 0; ii < qMin(args.size(), maxArgs); ++ii) {
        bool ok;
        vals[ii] = args[ii].toDouble(&ok);
        allOk &= ok;
    }
    if (!allOk)
        return QColor();

    QColor c;
    switch (spec) {
    case QColor::Rgb:
        c.setRgbF(vals[0], vals[1], vals[2], vals[3]);
        break;
    case QColor::Cmyk:
        c.setCmykF(vals[0], vals[1], vals[2], vals[3], vals[4]);
        break;
    case QColor::Hsv:
        c.setHsvF(vals[0], vals[1], vals[2], vals[3]);
        break;
    case QColor::Hsl:
        c.setHslF(vals[0], vals[1], vals[2], vals[3]);
        break;
    default:
        break;
    }
    return c;
}

static QColor colorFromFuncAndArgs(const QString &func, const QStringList &args)
{
    if (args.isEmpty())
        return QColor();

    if (args.size() < 3) {
        QString arg0 = removeWhitespace(args[0]);
        arg0.remove(QLatin1Char('\"'));
        if (func == QLatin1String("setNamedColor"))
            return QColor(arg0);

        if (arg0.startsWith(QLatin1Char('#')))
            return QColor(arg0);

        if (arg0.startsWith(QLatin1String("Qt::"))) {
            arg0.remove(0, 4);
            return fromEnumString(arg0);
        }

        return QColor();
    }

    QColor::Spec spec = specForFunc(func);
    if (spec == QColor::Invalid)
        return QColor();

    if (func.endsWith(QLatin1Char('F')))
        return colorFromArgsF(args, spec);

    return colorFromArgs(args, spec);
}

void ColorPreviewHoverHandler::identifyMatch(TextEditorWidget *editorWidget, int pos)
{
    if (editorWidget->extraSelectionTooltip(pos).isEmpty()) {
        const QTextBlock tb = editorWidget->document()->findBlock(pos);
        const int tbpos = pos - tb.position();
        const QString tbtext = tb.text();

        QString colorString = extractColorString(tbtext, tbpos);
        m_colorTip = checkColorText(colorString);

        if (!m_colorTip.isValid()) {
            QString funcName;
            QStringList args;
            if (extractFuncAndArgs(tbtext, tbpos, funcName, args))
                m_colorTip = colorFromFuncAndArgs(funcName, args);
        }

        setPriority(m_colorTip.isValid() ? Priority_Help - 1 : Priority_None);
    }
}

void ColorPreviewHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (m_colorTip.isValid())
        Utils::ToolTip::show(point, m_colorTip, editorWidget);
    else
        Utils::ToolTip::hide();
}

} // namespace TextEditor
