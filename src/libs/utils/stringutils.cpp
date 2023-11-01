// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stringutils.h"

#include "filepath.h"
#include "qtcassert.h"
#include "stylehelper.h"
#include "theme/theme.h"
#include "utilstr.h"

#ifdef QT_WIDGETS_LIB
#include <QApplication>
#include <QClipboard>
#endif

#include <QCollator>
#include <QDir>
#include <QFontMetrics>
#include <QJsonArray>
#include <QJsonValue>
#include <QLocale>
#include <QPalette>
#include <QRegularExpression>
#include <QSet>
#include <QTextDocument>
#include <QTextList>
#include <QTime>

#include <limits.h>

namespace Utils {

QTCREATOR_UTILS_EXPORT QString settingsKey(const QString &category)
{
    QString rc(category);
    const QChar underscore = '_';
    // Remove the sort category "X.Category" -> "Category"
    if (rc.size() > 2 && rc.at(0).isLetter() && rc.at(1) == '.')
        rc.remove(0, 2);
    // Replace special characters
    const int size = rc.size();
    for (int i = 0; i < size; i++) {
        const QChar c = rc.at(i);
        if (!c.isLetterOrNumber() && c != underscore)
            rc[i] = underscore;
    }
    return rc;
}

// Figure out length of common start of string ("C:\a", "c:\b"  -> "c:\"
static inline int commonPartSize(const QString &s1, const QString &s2)
{
    const int size = qMin(s1.size(), s2.size());
    for (int i = 0; i < size; i++)
        if (s1.at(i) != s2.at(i))
            return i;
    return size;
}

QTCREATOR_UTILS_EXPORT QString commonPrefix(const QStringList &strings)
{
    switch (strings.size()) {
    case 0:
        return QString();
    case 1:
        return strings.front();
    default:
        break;
    }
    // Figure out common string part: "C:\foo\bar1" "C:\foo\bar2"  -> "C:\foo\bar"
    int commonLength = INT_MAX;
    const int last = strings.size() - 1;
    for (int i = 0; i < last; i++)
        commonLength = qMin(commonLength, commonPartSize(strings.at(i), strings.at(i + 1)));
    if (!commonLength)
        return QString();
    return strings.at(0).left(commonLength);
}

static bool validateVarName(const QString &varName)
{
    return !varName.startsWith("JS:");
}

bool AbstractMacroExpander::expandNestedMacros(const QString &str, int *pos, QString *ret)
{
    QString varName;
    QString pattern, replace;
    QString defaultValue;
    QString *currArg = &varName;
    QChar prev;
    QChar c;
    QChar replacementChar;
    bool replaceAll = false;

    int i = *pos;
    int strLen = str.length();
    varName.reserve(strLen - i);
    for (; i < strLen; prev = c) {
        c = str.at(i++);
        if (c == '\\' && i < strLen) {
            c = str.at(i++);
            // For the replacement, do not skip the escape sequence when followed by a digit.
            // This is needed for enabling convenient capture group replacement,
            // like %{var/(.)(.)/\2\1}, without escaping the placeholders.
            if (currArg == &replace && c.isDigit())
                *currArg += '\\';
            *currArg += c;
        } else if (c == '}') {
            if (varName.isEmpty()) { // replace "%{}" with "%"
                *ret = QString('%');
                *pos = i;
                return true;
            }
            QSet<AbstractMacroExpander*> seen;
            if (resolveMacro(varName, ret, seen)) {
                *pos = i;
                if (!pattern.isEmpty() && currArg == &replace) {
                    const QRegularExpression regexp(pattern);
                    if (regexp.isValid()) {
                        if (replaceAll) {
                            ret->replace(regexp, replace);
                        } else {
                            // There isn't an API for replacing once...
                            const QRegularExpressionMatch match = regexp.match(*ret);
                            if (match.hasMatch()) {
                                *ret = ret->left(match.capturedStart(0))
                                        + match.captured(0).replace(regexp, replace)
                                        + ret->mid(match.capturedEnd(0));
                            }
                        }
                    }
                }
                return true;
            }
            if (!defaultValue.isEmpty()) {
                *pos = i;
                *ret = defaultValue;
                return true;
            }
            return false;
        } else if (c == '{' && prev == '%') {
            if (!expandNestedMacros(str, &i, ret))
                return false;
            varName.chop(1);
            varName += *ret;
        } else if (currArg == &varName && c == '-' && prev == ':' && validateVarName(varName)) {
            varName.chop(1);
            currArg = &defaultValue;
        } else if (currArg == &varName && (c == '/' || c == '#') && validateVarName(varName)) {
            replacementChar = c;
            currArg = &pattern;
            if (i < strLen && str.at(i) == replacementChar) {
                ++i;
                replaceAll = true;
            }
        } else if (currArg == &pattern && c == replacementChar) {
            currArg = &replace;
        } else {
            *currArg += c;
        }
    }
    return false;
}

int AbstractMacroExpander::findMacro(const QString &str, int *pos, QString *ret)
{
    forever {
        int openPos = str.indexOf("%{", *pos);
        if (openPos < 0)
            return 0;
        int varPos = openPos + 2;
        if (expandNestedMacros(str, &varPos, ret)) {
            *pos = openPos;
            return varPos - openPos;
        }
        // An actual expansion may be nested into a "false" one,
        // so we continue right after the last %{.
        *pos = openPos + 2;
    }
}

QTCREATOR_UTILS_EXPORT void expandMacros(QString *str, AbstractMacroExpander *mx)
{
    QString rsts;

    for (int pos = 0; int len = mx->findMacro(*str, &pos, &rsts); ) {
        str->replace(pos, len, rsts);
        pos += rsts.length();
    }
}

QTCREATOR_UTILS_EXPORT QString expandMacros(const QString &str, AbstractMacroExpander *mx)
{
    QString ret = str;
    expandMacros(&ret, mx);
    return ret;
}

QTCREATOR_UTILS_EXPORT QString stripAccelerator(const QString &text)
{
    QString res = text;
    for (int index = res.indexOf('&'); index != -1; index = res.indexOf('&', index + 1))
        res.remove(index, 1);
    return res;
}

QTCREATOR_UTILS_EXPORT bool readMultiLineString(const QJsonValue &value, QString *out)
{
    QTC_ASSERT(out, return false);
    if (value.isString()) {
        *out = value.toString();
    } else if (value.isArray()) {
        QJsonArray array = value.toArray();
        QStringList lines;
        for (const QJsonValue &v : array) {
            if (!v.isString())
                return false;
            lines.append(v.toString());
        }
        *out = lines.join(QLatin1Char('\n'));
    } else {
        return false;
    }
    return true;
}

QTCREATOR_UTILS_EXPORT int parseUsedPortFromNetstatOutput(const QByteArray &line)
{
    const QByteArray trimmed = line.trimmed();
    int base = 0;
    QByteArray portString;

    if (trimmed.startsWith("TCP") || trimmed.startsWith("UDP")) {
        // Windows.  Expected output is something like
        //
        // Active Connections
        //
        //   Proto  Local Address          Foreign Address        State
        //   TCP    0.0.0.0:80             0.0.0.0:0              LISTENING
        //   TCP    0.0.0.0:113            0.0.0.0:0              LISTENING
        // [...]
        //   TCP    10.9.78.4:14714       0.0.0.0:0              LISTENING
        //   TCP    10.9.78.4:50233       12.13.135.180:993      ESTABLISHED
        // [...]
        //   TCP    [::]:445               [::]:0                 LISTENING
        //   TCP    192.168.0.80:51905     169.55.74.50:443       ESTABLISHED
        //   UDP    [fe80::880a:2932:8dff:a858%6]:1900  *:*
        const int firstBracketPos = trimmed.indexOf('[');
        int colonPos = -1;
        if (firstBracketPos == -1) {
            colonPos = trimmed.indexOf(':');  // IPv4
        } else  {
            // jump over host part
            const int secondBracketPos = trimmed.indexOf(']', firstBracketPos + 1);
            colonPos = trimmed.indexOf(':', secondBracketPos);
        }
        const int firstDigitPos = colonPos + 1;
        const int spacePos = trimmed.indexOf(' ', firstDigitPos);
        if (spacePos < 0)
            return -1;
        const int len = spacePos - firstDigitPos;
        base = 10;
        portString = trimmed.mid(firstDigitPos, len);
    } else if (trimmed.startsWith("tcp") || trimmed.startsWith("udp")) {
        // macOS. Expected output is something like
        //
        // Active Internet connections (including servers)
        // Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)
        // tcp4       0      0  192.168.1.12.55687     88.198.14.66.443       ESTABLISHED
        // tcp6       0      0  2a01:e34:ee42:d0.55684 2a02:26f0:ff::5c.443   ESTABLISHED
        // [...]
        // tcp4       0      0  *.631                  *.*                    LISTEN
        // tcp6       0      0  *.631                  *.*                    LISTEN
        // [...]
        // udp4       0      0  192.168.79.1.123       *.*
        // udp4       0      0  192.168.8.1.123        *.*
        int firstDigitPos = -1;
        int spacePos = -1;
        if (trimmed[3] == '6') {
            // IPV6
            firstDigitPos = trimmed.indexOf('.') + 1;
            spacePos = trimmed.indexOf(' ', firstDigitPos);
        } else {
            // IPV4
            firstDigitPos = trimmed.indexOf('.') + 1;
            spacePos = trimmed.indexOf(' ', firstDigitPos);
            firstDigitPos = trimmed.lastIndexOf('.', spacePos) + 1;
        }
        if (spacePos < 0)
            return -1;
        base = 10;
        portString = trimmed.mid(firstDigitPos, spacePos - firstDigitPos);
        if (portString == "*")
            return -1;
    } else {
        // Expected output on Linux something like
        //
        //   sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt ...
        //   0: 00000000:2805 00000000:0000 0A 00000000:00000000 00:00000000 00000000  ...
        //
        const int firstColonPos = trimmed.indexOf(':');
        if (firstColonPos < 0)
            return -1;
        const int secondColonPos = trimmed.indexOf(':', firstColonPos + 1);
        if (secondColonPos < 0)
            return -1;
        const int spacePos = trimmed.indexOf(' ', secondColonPos + 1);
        if (spacePos < 0)
            return -1;
        const int len = spacePos - secondColonPos - 1;
        base = 16;
        portString = trimmed.mid(secondColonPos + 1, len);
    }

    bool ok = true;
    const int port = portString.toInt(&ok, base);
    if (!ok) {
        qWarning("%s: Unexpected string '%s' is not a port. Tried to read from '%s'",
                Q_FUNC_INFO, line.data(), portString.data());
        return -1;
    }
    return port;
}

int caseFriendlyCompare(const QString &a, const QString &b)
{
    static const auto makeCollator = [](Qt::CaseSensitivity caseSensitivity) {
        QCollator collator;
        collator.setNumericMode(true);
        collator.setCaseSensitivity(caseSensitivity);
        return collator;
    };
    static const QCollator insensitiveCollator = makeCollator(Qt::CaseInsensitive);
    const int result = insensitiveCollator.compare(a, b);
    if (result != 0)
        return result;
    static const QCollator sensitiveCollator = makeCollator(Qt::CaseSensitive);
    return sensitiveCollator.compare(a, b);
}

QString quoteAmpersands(const QString &text)
{
    QString result = text;
    return result.replace("&", "&&");
}

QString formatElapsedTime(qint64 elapsed)
{
    elapsed += 500; // round up
    const QString format = QString::fromLatin1(elapsed >= 3600000 ? "h:mm:ss" : "mm:ss");
    const QString time = QTime(0, 0).addMSecs(elapsed).toString(format);
    return Tr::tr("Elapsed time: %1.").arg(time);
}

/*
 * Basically QRegularExpression::wildcardToRegularExpression(), but let wildcards match
 * path separators as well
 */
QString wildcardToRegularExpression(const QString &original)
{
    const qsizetype wclen = original.size();
    QString rx;
    rx.reserve(wclen + wclen / 16);
    qsizetype i = 0;
    const QChar *wc = original.data();

    const QLatin1String starEscape(".*");
    const QLatin1String questionMarkEscape(".");

    while (i < wclen) {
        const QChar c = wc[i++];
        switch (c.unicode()) {
        case '*':
            rx += starEscape;
            break;
        case '?':
            rx += questionMarkEscape;
            break;
        case '\\':
        case '$':
        case '(':
        case ')':
        case '+':
        case '.':
        case '^':
        case '{':
        case '|':
        case '}':
            rx += QLatin1Char('\\');
            rx += c;
            break;
        case '[':
            rx += c;
            // Support for the [!abc] or [!a-c] syntax
            if (i < wclen) {
                if (wc[i] == QLatin1Char('!')) {
                    rx += QLatin1Char('^');
                    ++i;
                }

                if (i < wclen && wc[i] == QLatin1Char(']'))
                    rx += wc[i++];

                while (i < wclen && wc[i] != QLatin1Char(']')) {
                    if (wc[i] == QLatin1Char('\\'))
                        rx += QLatin1Char('\\');
                    rx += wc[i++];
                }
            }
            break;
        default:
            rx += c;
            break;
        }
    }

    return QRegularExpression::anchoredPattern(rx);
}

QTCREATOR_UTILS_EXPORT QString languageNameFromLanguageCode(const QString &languageCode)
{
    QLocale locale(languageCode);
    QString languageName = QLocale::languageToString(locale.language());
    QString nativeLanguageName = locale.nativeLanguageName().simplified();

    if (!nativeLanguageName.isEmpty() && languageName != nativeLanguageName)
        languageName += " - " + locale.nativeLanguageName();
    return languageName;
}

#ifdef QT_WIDGETS_LIB
QTCREATOR_UTILS_EXPORT void setClipboardAndSelection(const QString &text)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
    if (clipboard->supportsSelection())
        clipboard->setText(text, QClipboard::Selection);
}
#endif

QTCREATOR_UTILS_EXPORT QString chopIfEndsWith(QString str, QChar c)
{
    if (str.endsWith(c))
        str.chop(1);

    return str;
}

QTCREATOR_UTILS_EXPORT QStringView chopIfEndsWith(QStringView str, QChar c)
{
    if (str.endsWith(c))
        str.chop(1);

    return str;
}

QTCREATOR_UTILS_EXPORT QString normalizeNewlines(const QString &text)
{
    QString res = text;
    const auto newEnd = std::unique(res.begin(), res.end(), [](const QChar c1, const QChar c2) {
        return c1 == '\r' && c2 == '\r'; // QTCREATORBUG-24556
    });
    res.chop(std::distance(newEnd, res.end()));
    res.replace("\r\n", "\n");
    return res;
}

/*!
    Joins all the not empty string list's \a strings into a single string with each element
    separated by the given \a separator (which can be an empty string).
*/
QTCREATOR_UTILS_EXPORT QString joinStrings(const QStringList &strings, QChar separator)
{
    QString result;
    for (const QString &string : strings) {
        if (string.isEmpty())
            continue;
        if (!result.isEmpty())
            result += separator;
        result += string;
    }
    return result;
}

/*!
    Returns a copy of \a string that has \a ch characters removed from the start.
*/
QTCREATOR_UTILS_EXPORT QString trimFront(const QString &string, QChar ch)
{
    const int size = string.size();
    int i = 0;
    while (i < size) {
        if (string.at(i) != ch)
            break;
        ++i;
    }
    if (i == 0)
        return string;
    if (i == size)
        return {};
    return string.mid(i);
}

/*!
    Returns a copy of \a string that has \a ch characters removed from the end.
*/
QTCREATOR_UTILS_EXPORT QString trimBack(const QString &string, QChar ch)
{
    const int size = string.size();
    int i = 0;
    while (i < size) {
        if (string.at(size - i - 1) != ch)
            break;
        ++i;
    }
    if (i == 0)
        return string;
    if (i == size)
        return {};
    return string.chopped(i);
}

/*!
    Returns a copy of \a string that has \a ch characters removed from the start and the end.
*/
QTCREATOR_UTILS_EXPORT QString trim(const QString &string, QChar ch)
{
    return trimFront(trimBack(string, ch), ch);
}

QTCREATOR_UTILS_EXPORT QString appendHelper(const QString &base, int n)
{
    return base + QString::number(n);
}

QTCREATOR_UTILS_EXPORT FilePath appendHelper(const FilePath &base, int n)
{
    return base.stringAppended(QString::number(n));
}

QTCREATOR_UTILS_EXPORT QPair<QStringView, QStringView> splitAtFirst(const QStringView &stringView,
                                                                    QChar ch)
{
    int splitIdx = stringView.indexOf(ch);
    if (splitIdx == -1)
        return {stringView, {}};

    QStringView left = stringView.mid(0, splitIdx);
    QStringView right = stringView.mid(splitIdx + 1);

    return {left, right};
}

QTCREATOR_UTILS_EXPORT QPair<QStringView, QStringView> splitAtFirst(const QString &string, QChar ch)
{
    QStringView view = string;
    return splitAtFirst(view, ch);
}

QTCREATOR_UTILS_EXPORT int endOfNextWord(const QString &string, int position)
{
    QTC_ASSERT(string.size() > position, return -1);

    static const QString wordSeparators = QStringLiteral(" \t\n\r()[]{}<>");

    const auto predicate = [](const QChar &c) { return wordSeparators.contains(c); };

    auto it = string.begin() + position;
    if (predicate(*it))
        it = std::find_if_not(it, string.end(), predicate);

    if (it == string.end())
        return -1;

    it = std::find_if(it, string.end(), predicate);
    if (it == string.end())
        return -1;

    return std::distance(string.begin(), it);
}

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , h2Brush(Qt::NoBrush)
    , m_codeBgBrush(Qt::NoBrush)
{
    parent->setIndentWidth(30); // default value is 40
}

QBrush MarkdownHighlighter::codeBgBrush()
{
    if (m_codeBgBrush.style() == Qt::NoBrush) {
        m_codeBgBrush = StyleHelper::mergedColors(QGuiApplication::palette().color(QPalette::Text),
                                                  QGuiApplication::palette().color(QPalette::Base),
                                                  10);
    }
    return m_codeBgBrush;
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    const QTextBlock block = currentBlock();
    QTextBlockFormat fmt = block.blockFormat();
    QTextCursor cur(currentBlock());
    if (fmt.hasProperty(QTextFormat::HeadingLevel)) {
        fmt.setTopMargin(10);
        fmt.setBottomMargin(10);

        // Draw an underline for Heading 2, by creating a texture brush
        // with the last pixel visible
        if (fmt.property(QTextFormat::HeadingLevel) == 2) {
            QTextCharFormat charFmt = currentBlock().charFormat();
            charFmt.setBaselineOffset(15);
            setFormat(0, text.length(), charFmt);

            if (h2Brush.style() == Qt::NoBrush) {
                const int height = QFontMetrics(charFmt.font()).height();
                QImage image(1, height, QImage::Format_ARGB32);

                image.fill(QColor(0, 0, 0, 0).rgba());
                image.setPixel(0,
                               height - 1,
                               Utils::creatorTheme()->color(Theme::TextColorDisabled).rgba());

                h2Brush = QBrush(image);
            }
            fmt.setBackground(h2Brush);
        }
        cur.setBlockFormat(fmt);
    } else if (fmt.hasProperty(QTextFormat::BlockCodeLanguage) && fmt.indent() == 0) {
        // set identation and background for code blocks
        fmt.setBackground(codeBgBrush());
        fmt.setIndent(1);
        cur.setBlockFormat(fmt);
    }

    // Show the bulet points as filled circles
    QTextList *list = cur.currentList();
    if (list) {
        QTextListFormat listFmt = list->format();
        if (listFmt.indent() == 1 && listFmt.style() == QTextListFormat::ListCircle) {
            listFmt.setStyle(QTextListFormat::ListDisc);
            list->setFormat(listFmt);
        }
    }

    // background color of code
    for (auto it = block.begin(); it != block.end(); ++it) {
        const QTextFragment fragment = it.fragment();
        QTextCharFormat fmt = fragment.charFormat();
        if (fmt.fontFixedPitch()) {
            fmt.setBackground(codeBgBrush());
            setFormat(fragment.position() - block.position(), fragment.length(), fmt);
        }
    }
}

} // namespace Utils
