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
#include <QTextCodec>
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
        const QJsonArray array = value.toArray();
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

enum class Base { Dec, Hex };

static bool isHex(const QChar &c)
{
    return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool isDigit(const QChar &c, Base base)
{
    if (base == Base::Hex && isHex(c))
        return true;
    return c.isDigit();
}

static int trailingNumber(const QString &line, Base base = Base::Dec)
{
    int lastNumberPos = line.size();
    while (lastNumberPos > 0) {
        if (!isDigit(line.at(lastNumberPos - 1), base))
            break;
        --lastNumberPos;
    }
    bool ok = true;
    const int port = line.mid(lastNumberPos).toInt(&ok, base == Base::Dec ? 10 : 16);
    return ok ? port : -1;
}

/*

Parsing algo is simple:
Depending on the value of 1st column we detect whether it's Win, Mac / Android / Qnx, or Linux
output.

In case of Win or Linux, we select the 2nd column for port parsing, otherwise we select
the 4th column.

For selected column we parse the trailing digits. In case of Linux we take into account hex digits.

Expected output (see tst_StringUtils::testParseUsedPortFromNetstatOutput_data()):

    === Windows ===

    Active Connections

    Proto  Local Address          Foreign Address        State
    TCP    0.0.0.0:80             0.0.0.0:0              LISTENING
    TCP    0.0.0.0:113            0.0.0.0:0              LISTENING
    [...]
    TCP    10.9.78.4:14714        0.0.0.0:0              LISTENING
    TCP    10.9.78.4:50233        12.13.135.180:993      ESTABLISHED
    [...]
    TCP    [::]:445               [::]:0                 LISTENING
    TCP    192.168.0.80:51905     169.55.74.50:443       ESTABLISHED
    UDP    [fe80::880a:2932:8dff:a858%6]:1900  *:*

    === Mac ===

    Active Internet connections (including servers)
    Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)
    tcp4       0      0  192.168.1.12.55687     88.198.14.66.443       ESTABLISHED
    tcp6       0      0  2a01:e34:ee42:d0.55684 2a02:26f0:ff::5c.443   ESTABLISHED
    [...]
    tcp4       0      0  *.631                  *.*                    LISTEN
    tcp6       0      0  *.631                  *.*                    LISTEN
    [...]
    udp4       0      0  192.168.79.1.123       *.*
    udp4       0      0  192.168.8.1.123        *.*

    === QNX ===

    Active Internet connections (including servers)
    Proto Recv-Q Send-Q  Local Address          Foreign Address        State
    tcp        0      0  10.9.7.5.22          10.9.7.4.46592       ESTABLISHED
    tcp        0      0  *.8000                 *.*                    LISTEN
    tcp        0      0  *.22                   *.*                    LISTEN
    udp        0      0  *.*                    *.*
    udp        0      0  *.*                    *.*
    Active Internet6 connections (including servers)
    Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)
    tcp6       0      0  *.22                   *.*                    LISTEN

    === Android ===

    tcp        0      0 10.0.2.16:49088         142.250.180.74:443      ESTABLISHED
    tcp        0      0 10.0.2.16:48380         142.250.186.196:443     CLOSE_WAIT
    tcp6       0      0 [::]:5555               [::]:*                  LISTEN
    tcp6       0      0 ::ffff:127.0.0.1:39417  [::]:*                  LISTEN
    tcp6       0      0 ::ffff:10.0.2.16:35046  ::ffff:142.250.203.:443 ESTABLISHED
    tcp6       0      0 ::ffff:127.0.0.1:46265  ::ffff:127.0.0.1:33155  TIME_WAIT
    udp        0      0 10.0.2.16:50950         142.250.75.14:443       ESTABLISHED
    udp     2560      0 10.0.2.16:68            10.0.2.2:67             ESTABLISHED
    udp        0      0 0.0.0.0:5353            0.0.0.0:*
    udp6       0      0 [::]:36662              [::]:*

    === Linux ===

    sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt ...
    0: 00000000:2805 00000000:0000 0A 00000000:00000000 00:00000000 00000000  ...
*/
QTCREATOR_UTILS_EXPORT int parseUsedPortFromNetstatOutput(const QByteArray &line)
{
    const QStringList columns = QString::fromUtf8(line).split(' ', Qt::SkipEmptyParts);
    if (columns.size() < 3)
        return -1;

    const QString firstColumn = columns.first();
    QString columnToParse;
    Base base = Base::Dec;

    if (firstColumn.startsWith("TCP") || firstColumn.startsWith("UDP")) { // Windows
        columnToParse = columns.at(1);
    } else if (firstColumn.startsWith("tcp") || firstColumn.startsWith("udp")) { // Mac, Android, Qnx
        if (columns.size() < 4)
            return -1;
        columnToParse = columns.at(3);
    } else if (firstColumn.size() > 1 && firstColumn.at(1) == ':') { // Linux
        columnToParse = columns.at(1);
        base = Base::Hex;
    } else {
        return -1;
    }

    if (columnToParse.size() > 0 && columnToParse.back() == '*')
        return -1; // Valid case, no warning. See QNX udp case.

    const int port = trailingNumber(columnToParse, base);
    if (port == -1) {
        qWarning("%s: Unexpected string '%s' is not a port. Tried to read from '%s'",
                 Q_FUNC_INFO, line.data(), columnToParse.toUtf8().data());
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

QString asciify(const QString &input)
{
    QString result;
    result.reserve(input.size() * 5);
    for (const QChar &c : input) {
        if (c.isPrint() && c.unicode() < 128)
            result += c;
        else
            result += QString::asprintf("u%04x", c.unicode());
    }
    return result;
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

QTCREATOR_UTILS_EXPORT QString normalizeNewlines(const QStringView &text)
{
    QString res = text.toString();
    const auto newEnd = std::unique(res.rbegin(), res.rend(), [](const QChar c1, const QChar c2) {
        return c1 == '\n' && c2 == '\r'; // QTCREATORBUG-24556
    });
    res.remove(0, std::distance(newEnd, res.rend()));
    return res;
}

QTCREATOR_UTILS_EXPORT QByteArray normalizeNewlines(const QByteArray &text)
{
    QByteArray res = text;
    const auto newEnd = std::unique(res.rbegin(), res.rend(), [](const char c1, const char c2) {
        return c1 == '\n' && c2 == '\r'; // QTCREATORBUG-24556
    });
    res.remove(0, std::distance(newEnd, res.rend()));
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
                               Utils::creatorColor(Theme::TextColorDisabled).rgba());

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

QString ansiColoredText(const QString &text, const QColor &color)
{
    static const QString formatString("\033[38;2;%1;%2;%3m%4\033[0m");
    return formatString.arg(color.red()).arg(color.green()).arg(color.blue()).arg(text);
}

QByteArray codecForLocale()
{
    if (QTextCodec *codec = QTextCodec::codecForLocale())
        return codec->name();
    QTC_CHECK(false);
    return {};
}

} // namespace Utils
