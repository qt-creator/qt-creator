/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerprotocol.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QRegExp>
#include <QTimeZone>

#include <ctype.h>

#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) qDebug("SOFT ASSERT: \"" cond"\" in file " __FILE__ ", line " QTC_ASSERT_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (cond) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)

namespace Debugger {
namespace Internal {

uchar fromhex(uchar c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return 10 + c - 'a';
    if (c >= 'A' && c <= 'Z')
        return 10 + c - 'A';
    return UCHAR_MAX;
}

void skipCommas(const char *&from, const char *to)
{
    while (*from == ',' && from != to)
        ++from;
}

void GdbMi::parseResultOrValue(const char *&from, const char *to)
{
    while (from != to && isspace(*from))
        ++from;

    //qDebug() << "parseResultOrValue: " << QByteArray(from, to - from);
    parseValue(from, to);
    if (isValid()) {
        //qDebug() << "no valid result in " << QByteArray(from, to - from);
        return;
    }
    if (from == to || *from == '(')
        return;
    const char *ptr = from;
    while (ptr < to && *ptr != '=') {
        //qDebug() << "adding" << QChar(*ptr) << "to name";
        ++ptr;
    }
    m_name = QByteArray(from, ptr - from);
    from = ptr;
    if (from < to && *from == '=') {
        ++from;
        parseValue(from, to);
    }
}

QByteArray GdbMi::parseCString(const char *&from, const char *to)
{
    QByteArray result;
    //qDebug() << "parseCString: " << QByteArray(from, to - from);
    if (*from != '"') {
        qDebug() << "MI Parse Error, double quote expected";
        ++from; // So we don't hang
        return QByteArray();
    }
    const char *ptr = from;
    ++ptr;
    while (ptr < to) {
        if (*ptr == '"') {
            ++ptr;
            result = QByteArray(from + 1, ptr - from - 2);
            break;
        }
        if (*ptr == '\\') {
            ++ptr;
            if (ptr == to) {
                qDebug() << "MI Parse Error, unterminated backslash escape";
                from = ptr; // So we don't hang
                return QByteArray();
            }
        }
        ++ptr;
    }
    from = ptr;

    int idx = result.indexOf('\\');
    if (idx >= 0) {
        char *dst = result.data() + idx;
        const char *src = dst + 1, *end = result.data() + result.length();
        do {
            char c = *src++;
            switch (c) {
                case 'a': *dst++ = '\a'; break;
                case 'b': *dst++ = '\b'; break;
                case 'f': *dst++ = '\f'; break;
                case 'n': *dst++ = '\n'; break;
                case 'r': *dst++ = '\r'; break;
                case 't': *dst++ = '\t'; break;
                case 'v': *dst++ = '\v'; break;
                case '"': *dst++ = '"'; break;
                case '\\': *dst++ = '\\'; break;
                case 'x': {
                        c = *src++;
                        int chars = 0;
                        uchar prod = 0;
                        while (true) {
                            uchar val = fromhex(c);
                            if (val == UCHAR_MAX)
                                break;
                            prod = prod * 16 + val;
                            if (++chars == 3 || src == end)
                                break;
                            c = *src++;
                        }
                        if (!chars) {
                            qDebug() << "MI Parse Error, unrecognized hex escape";
                            return QByteArray();
                        }
                        *dst++ = prod;
                        break;
                    }
                default:
                    {
                        int chars = 0;
                        uchar prod = 0;
                        forever {
                            if (c < '0' || c > '7') {
                                --src;
                                break;
                            }
                            prod = prod * 8 + c - '0';
                            if (++chars == 3 || src == end)
                                break;
                            c = *src++;
                        }
                        if (!chars) {
                            qDebug() << "MI Parse Error, unrecognized backslash escape";
                            return QByteArray();
                        }
                        *dst++ = prod;
                    }
            }
            while (src != end) {
                char c = *src++;
                if (c == '\\')
                    break;
                *dst++ = c;
            }
        } while (src != end);
        *dst = 0;
        result.truncate(dst - result.data());
    }

    return result;
}

void GdbMi::parseValue(const char *&from, const char *to)
{
    //qDebug() << "parseValue: " << QByteArray(from, to - from);
    switch (*from) {
        case '{':
            parseTuple(from, to);
            break;
        case '[':
            parseList(from, to);
            break;
        case '"':
            m_type = Const;
            m_data = parseCString(from, to);
            break;
        default:
            break;
    }
}


void GdbMi::parseTuple(const char *&from, const char *to)
{
    //qDebug() << "parseTuple: " << QByteArray(from, to - from);
    //QTC_CHECK(*from == '{');
    ++from;
    parseTuple_helper(from, to);
}

void GdbMi::parseTuple_helper(const char *&from, const char *to)
{
    skipCommas(from, to);
    //qDebug() << "parseTuple_helper: " << QByteArray(from, to - from);
    m_type = Tuple;
    while (from < to) {
        if (*from == '}') {
            ++from;
            break;
        }
        GdbMi child;
        child.parseResultOrValue(from, to);
        //qDebug() << "\n=======\n" << qPrintable(child.toString()) << "\n========\n";
        if (!child.isValid())
            return;
        m_children.push_back(child);
        skipCommas(from, to);
    }
}

void GdbMi::parseList(const char *&from, const char *to)
{
    //qDebug() << "parseList: " << QByteArray(from, to - from);
    //QTC_CHECK(*from == '[');
    ++from;
    m_type = List;
    skipCommas(from, to);
    while (from < to) {
        if (*from == ']') {
            ++from;
            break;
        }
        GdbMi child;
        child.parseResultOrValue(from, to);
        if (child.isValid())
            m_children.push_back(child);
        skipCommas(from, to);
    }
}

static QByteArray ind(int indent)
{
    return QByteArray(2 * indent, ' ');
}

void GdbMi::dumpChildren(QByteArray * str, bool multiline, int indent) const
{
    for (size_t i = 0; i < m_children.size(); ++i) {
        if (i != 0) {
            *str += ',';
            if (multiline)
                *str += '\n';
        }
        if (multiline)
            *str += ind(indent);
        *str += m_children.at(i).toString(multiline, indent);
    }
}

QByteArray GdbMi::escapeCString(const QByteArray &ba)
{
    QByteArray ret;
    ret.reserve(ba.length() * 2);
    for (int i = 0; i < ba.length(); ++i) {
        const uchar c = ba.at(i);
        switch (c) {
            case '\\': ret += "\\\\"; break;
            case '\a': ret += "\\a"; break;
            case '\b': ret += "\\b"; break;
            case '\f': ret += "\\f"; break;
            case '\n': ret += "\\n"; break;
            case '\r': ret += "\\r"; break;
            case '\t': ret += "\\t"; break;
            case '\v': ret += "\\v"; break;
            case '"': ret += "\\\""; break;
            default:
                if (c < 32 || c == 127) {
                    ret += '\\';
                    ret += ('0' + (c >> 6));
                    ret += ('0' + ((c >> 3) & 7));
                    ret += ('0' + (c & 7));
                } else {
                    ret += c;
                }
        }
    }
    return ret;
}

QByteArray GdbMi::toString(bool multiline, int indent) const
{
    QByteArray result;
    switch (m_type) {
        case Invalid:
            if (multiline)
                result += ind(indent) + "Invalid\n";
            else
                result += "Invalid";
            break;
        case Const:
            if (!m_name.isEmpty())
                result += m_name + '=';
            result += '"' + escapeCString(m_data) + '"';
            break;
        case Tuple:
            if (!m_name.isEmpty())
                result += m_name + '=';
            if (multiline) {
                result += "{\n";
                dumpChildren(&result, multiline, indent + 1);
                result += '\n' + ind(indent) + '}';
            } else {
                result += '{';
                dumpChildren(&result, multiline, indent + 1);
                result += '}';
            }
            break;
        case List:
            if (!m_name.isEmpty())
                result += m_name + '=';
            if (multiline) {
                result += "[\n";
                dumpChildren(&result, multiline, indent + 1);
                result += '\n' + ind(indent) + ']';
            } else {
                result += '[';
                dumpChildren(&result, multiline, indent + 1);
                result += ']';
            }
            break;
    }
    return result;
}

void GdbMi::fromString(const QByteArray &ba)
{
    const char *from = ba.constBegin();
    const char *to = ba.constEnd();
    parseResultOrValue(from, to);
}

void GdbMi::fromStringMultiple(const QByteArray &ba)
{
    const char *from = ba.constBegin();
    const char *to = ba.constEnd();
    parseTuple_helper(from, to);
}

const GdbMi &GdbMi::operator[](const char *name) const
{
    static GdbMi empty;
    for (int i = 0, n = int(m_children.size()); i < n; ++i)
        if (m_children.at(i).m_name == name)
            return m_children.at(i);
    return empty;
}

qulonglong GdbMi::toAddress() const
{
    QByteArray ba = m_data;
    if (ba.endsWith('L'))
        ba.chop(1);
    if (ba.startsWith('*') || ba.startsWith('@'))
        ba = ba.mid(1);
    return ba.toULongLong(0, 0);
}

//////////////////////////////////////////////////////////////////////////////////
//
// GdbResponse
//
//////////////////////////////////////////////////////////////////////////////////

QByteArray DebuggerResponse::stringFromResultClass(ResultClass resultClass)
{
    switch (resultClass) {
        case ResultDone: return "done";
        case ResultRunning: return "running";
        case ResultConnected: return "connected";
        case ResultError: return "error";
        case ResultExit: return "exit";
        default: return "unknown";
    }
}

QByteArray DebuggerResponse::toString() const
{
    QByteArray result;
    if (token != -1)
        result = QByteArray::number(token);
    result += '^';
    result += stringFromResultClass(resultClass);
    if (data.isValid())
        result += ',' + data.toString();
    result += '\n';
    return result;
}


//////////////////////////////////////////////////////////////////////////////////
//
// GdbResponse
//
//////////////////////////////////////////////////////////////////////////////////

// Tested in tests/auto/debugger/tst_gdb.cpp

void extractGdbVersion(const QString &msg,
    int *gdbVersion, int *gdbBuildVersion, bool *isMacGdb, bool *isQnxGdb)
{
    const QChar dot(QLatin1Char('.'));

    const bool ignoreParenthesisContent = msg.contains(QLatin1String("rubenvb"))
                                       || msg.contains(QLatin1String("openSUSE"));

    const QChar parOpen(QLatin1Char('('));
    const QChar parClose(QLatin1Char(')'));

    QString cleaned;
    QString build;
    bool inClean = true;
    bool inParenthesis = false;
    foreach (QChar c, msg) {
        if (inClean && !cleaned.isEmpty() && c != dot && (c.isPunct() || c.isSpace()))
            inClean = false;
        if (ignoreParenthesisContent) {
            if (!inParenthesis && c == parOpen)
                inParenthesis = true;
            if (inParenthesis && c == parClose)
                inParenthesis = false;
            if (inParenthesis)
                continue;
        }
        if (inClean) {
            if (c.isDigit())
                cleaned.append(c);
            else if (!cleaned.isEmpty() && !cleaned.endsWith(dot))
                cleaned.append(dot);
        } else {
            if (c.isDigit())
                build.append(c);
            else if (!build.isEmpty() && !build.endsWith(dot))
                build.append(dot);
        }
    }

    *isMacGdb = msg.contains(QLatin1String("Apple version"));
    *isQnxGdb = msg.contains(QLatin1String("qnx"));

    *gdbVersion = 10000 * cleaned.section(dot, 0, 0).toInt()
                  + 100 * cleaned.section(dot, 1, 1).toInt()
                    + 1 * cleaned.section(dot, 2, 2).toInt();
    if (cleaned.count(dot) >= 3)
        *gdbBuildVersion = cleaned.section(dot, 3, 3).toInt();
    else
        *gdbBuildVersion = build.section(dot, 0, 0).toInt();

    if (*isMacGdb)
        *gdbBuildVersion = build.section(dot, 1, 1).toInt();
}

//////////////////////////////////////////////////////////////////////////////////
//
// Decoding
//
//////////////////////////////////////////////////////////////////////////////////

static QString quoteUnprintableLatin1(const QByteArray &ba)
{
    QString res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const unsigned char c = ba.at(i);
        if (isprint(c)) {
            res += QLatin1Char(c);
        } else {
            qsnprintf(buf, sizeof(buf) - 1, "\\%x", int(c));
            res += QLatin1String(buf);
        }
    }
    return res;
}

static QDate dateFromData(int jd)
{
    return jd ? QDate::fromJulianDay(jd) : QDate();
}

static QTime timeFromData(int ms)
{
    return ms == -1 ? QTime() : QTime(0, 0, 0, 0).addMSecs(ms);
}

// Stolen and adapted from qdatetime.cpp
static void getDateTime(qint64 msecs, int status, QDate *date, QTime *time)
{
    enum {
        SECS_PER_DAY = 86400,
        MSECS_PER_DAY = 86400000,
        SECS_PER_HOUR = 3600,
        MSECS_PER_HOUR = 3600000,
        SECS_PER_MIN = 60,
        MSECS_PER_MIN = 60000,
        TIME_T_MAX = 2145916799,  // int maximum 2037-12-31T23:59:59 UTC
        JULIAN_DAY_FOR_EPOCH = 2440588 // result of julianDayFromDate(1970, 1, 1)
    };

    // Status of date/time
    enum StatusFlag {
        NullDate            = 0x01,
        NullTime            = 0x02,
        ValidDate           = 0x04,
        ValidTime           = 0x08,
        ValidDateTime       = 0x10,
        TimeZoneCached      = 0x20,
        SetToStandardTime   = 0x40,
        SetToDaylightTime   = 0x80
    };

    qint64 jd = JULIAN_DAY_FOR_EPOCH;
    qint64 ds = 0;

    if (qAbs(msecs) >= MSECS_PER_DAY) {
        jd += (msecs / MSECS_PER_DAY);
        msecs %= MSECS_PER_DAY;
    }

    if (msecs < 0) {
        ds = MSECS_PER_DAY - msecs - 1;
        jd -= ds / MSECS_PER_DAY;
        ds = ds % MSECS_PER_DAY;
        ds = MSECS_PER_DAY - ds - 1;
    } else {
        ds = msecs;
    }

    *date = (status & NullDate) ? QDate() : QDate::fromJulianDay(jd);
    *time = (status & NullTime) ? QTime() : QTime::fromMSecsSinceStartOfDay(ds);
}

QString decodeData(const QByteArray &ba, int encoding)
{
    switch (encoding) {
        case Unencoded8Bit: // 0
            return quoteUnprintableLatin1(ba);
        case Base64Encoded8BitWithQuotes: { // 1, used for QByteArray
            const QChar doubleQuote(QLatin1Char('"'));
            QString rc = doubleQuote;
            rc += quoteUnprintableLatin1(QByteArray::fromBase64(ba));
            rc += doubleQuote;
            return rc;
        }
        case Base64Encoded16BitWithQuotes: { // 2, used for QString
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            QString rc = doubleQuote;
            rc += QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
            rc += doubleQuote;
            return rc;
        }
        case Base64Encoded32BitWithQuotes: { // 3
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            const QChar doubleQuote(QLatin1Char('"'));
            QString rc = doubleQuote;
            rc += QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4);
            rc += doubleQuote;
            return rc;
        }
        case Base64Encoded16Bit: { // 4, without quotes (see 2)
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            return QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
        }
        case Base64Encoded8Bit: { // 5, without quotes (see 1)
            return quoteUnprintableLatin1(QByteArray::fromBase64(ba));
        }
        case Hex2EncodedLatin1WithQuotes: { // 6, %02x encoded 8 bit Latin1 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromLatin1(decodedBa, decodedBa.size()) + doubleQuote;
        }
        case Hex4EncodedLittleEndianWithQuotes: { // 7, %04x encoded 16 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2) + doubleQuote;
        }
        case Hex8EncodedLittleEndianWithQuotes: { // 8, %08x encoded 32 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4) + doubleQuote;
        }
        case Hex2EncodedUtf8WithQuotes: { // 9, %02x encoded 8 bit UTF-8 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromUtf8(decodedBa) + doubleQuote;
        }
        case Hex8EncodedBigEndian: { // 10, %08x encoded 32 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            QByteArray decodedBa = QByteArray::fromHex(ba);
            for (int i = 0; i < decodedBa.size() - 3; i += 4) {
                char c = decodedBa.at(i);
                decodedBa[i] = decodedBa.at(i + 3);
                decodedBa[i + 3] = c;
                c = decodedBa.at(i + 1);
                decodedBa[i + 1] = decodedBa.at(i + 2);
                decodedBa[i + 2] = c;
            }
            return doubleQuote + QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4) + doubleQuote;
        }
        case Hex4EncodedBigEndianWithQuotes: { // 11, %04x encoded 16 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            QByteArray decodedBa = QByteArray::fromHex(ba);
            for (int i = 0; i < decodedBa.size(); i += 2) {
                char c = decodedBa.at(i);
                decodedBa[i] = decodedBa.at(i + 1);
                decodedBa[i + 1] = c;
            }
            return doubleQuote + QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2) + doubleQuote;
        }
        case Hex4EncodedLittleEndianWithoutQuotes: { // 12, see 7, without quotes
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
        }
        case Hex2EncodedLocal8BitWithQuotes: { // 13, %02x encoded 8 bit UTF-8 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromLocal8Bit(decodedBa) + doubleQuote;
        }
        case JulianDate: { // 14, an integer count
            const QDate date = dateFromData(ba.toInt());
            return date.isValid() ? date.toString(Qt::TextDate) : QLatin1String("(invalid)");
        }
        case MillisecondsSinceMidnight: {
            const QTime time = timeFromData(ba.toInt());
            return time.isValid() ? time.toString(Qt::TextDate) : QLatin1String("(invalid)");
        }
        case JulianDateAndMillisecondsSinceMidnight: {
            const int p = ba.indexOf('/');
            const QDate date = dateFromData(ba.left(p).toInt());
            const QTime time = timeFromData(ba.mid(p + 1 ).toInt());
            const QDateTime dateTime = QDateTime(date, time);
            return dateTime.isValid() ? dateTime.toString(Qt::TextDate) : QLatin1String("(invalid)");
        }
        case IPv6AddressAndHexScopeId: { // 27, 16 hex-encoded bytes, "%" and the string-encoded scope
            const int p = ba.indexOf('%');
            QHostAddress ip6(QString::fromLatin1(p == -1 ? ba : ba.left(p)));
            if (ip6.isNull())
                break;

            const QByteArray scopeId = p == -1 ? QByteArray() : QByteArray::fromHex(ba.mid(p + 1));
            if (!scopeId.isEmpty())
                ip6.setScopeId(QString::fromUtf16(reinterpret_cast<const ushort *>(scopeId.constData()),
                                                  scopeId.length() / 2));
            return ip6.toString();
        }
        case Hex2EncodedUtf8WithoutQuotes: { // 28, %02x encoded 8 bit UTF-8 data without quotes
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return QString::fromUtf8(decodedBa);
        }
        case DateTimeInternal: { // 29, DateTimeInternal: msecs, spec, offset, tz, status
            int p0 = ba.indexOf('/');
            int p1 = ba.indexOf('/', p0 + 1);
            int p2 = ba.indexOf('/', p1 + 1);
            int p3 = ba.indexOf('/', p2 + 1);

            qint64 msecs = ba.left(p0).toLongLong();
            ++p0;
            Qt::TimeSpec spec = Qt::TimeSpec(ba.mid(p0, p1 - p0).toInt());
            ++p1;
            qulonglong offset = ba.mid(p1, p2 - p1).toInt();
            ++p2;
            QByteArray timeZoneId = QByteArray::fromHex(ba.mid(p2, p3 - p2));
            ++p3;
            int status = ba.mid(p3).toInt();

            QDate date;
            QTime time;
            getDateTime(msecs, status, &date, &time);

            QDateTime dateTime;
            if (spec == Qt::OffsetFromUTC) {
                dateTime = QDateTime(date, time, spec, offset);
            } else if (spec == Qt::TimeZone) {
                if (!QTimeZone::isTimeZoneIdAvailable(timeZoneId))
                    return QLatin1String("<unavailable>");
                dateTime = QDateTime(date, time, QTimeZone(timeZoneId));
            } else {
                dateTime = QDateTime(date, time, spec);
            }
            return dateTime.toString();
        }
        case Hex2EncodedFloat4: {
            const QByteArray s = QByteArray::fromHex(ba);
            QTC_ASSERT(s.size() == 4, break);
            union { char c[4]; float f; } u = { { s[3], s[2], s[1], s[0] } };
            return QString::number(u.f);
        }
        case Hex2EncodedFloat8: {
            const QByteArray s = QByteArray::fromHex(ba);
            QTC_ASSERT(s.size() == 8, break);
            union { char c[8]; double d; } u = { { s[7], s[6], s[5], s[4], s[3], s[2], s[1], s[0] } };
            return QString::number(u.d);
        }
        case SpecialEmptyValue: {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<empty>");
        }
        case SpecialUninitializedValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<uninitialized>");
        }
        case SpecialInvalidValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<invalid>");
        }
        case SpecialNotAccessibleValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<not accessible>");
        }
        case SpecialItemCountValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<%n items>", 0, ba.toInt());
        }
        case SpecialMinimumItemCountValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<at least %n items>", 0, ba.toInt());
        }
        case SpecialNotCallableValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<not callable>");
        }
        case SpecialNullReferenceValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<null reference>");
        }
        case SpecialOptimizedOutValue:  {
            return QCoreApplication::translate("Debugger::Internal::WatchHandler", "<optimized out>");
        }
        case SpecialEmptyStructureValue:  {
            return QLatin1String("{...}");
        }
    }
    qDebug() << "ENCODING ERROR: " << encoding;
    return QCoreApplication::translate("Debugger", "<Encoding error>");
}

//////////////////////////////////////////////////////////////////////////////////
//
// DebuggerCommand
//
//////////////////////////////////////////////////////////////////////////////////

void DebuggerCommand::argHelper(const char *name, const QByteArray &data)
{
    args.append('"');
    args.append(name);
    args.append("\":");
    args.append(data);
    args.append(",");
}

void DebuggerCommand::arg(const char *name, int value)
{
    argHelper(name, QByteArray::number(value));
}

void DebuggerCommand::arg(const char *name, qlonglong value)
{
    argHelper(name, QByteArray::number(value));
}

void DebuggerCommand::arg(const char *name, qulonglong value)
{
    argHelper(name, QByteArray::number(value));
}

void DebuggerCommand::arg(const char *name, const QString &value)
{
    arg(name, value.toUtf8().data());
}

void DebuggerCommand::arg(const char *name, const QByteArray &value)
{
    arg(name, value.data());
}

void DebuggerCommand::arg(const char *name, const char *value)
{
    args.append('"');
    args.append(name);
    args.append("\":\"");
    args.append(value);
    args.append("\",");
}

void DebuggerCommand::arg(const char *name, const QList<int> &list)
{
    beginList(name);
    foreach (int item, list) {
        args.append(QByteArray::number(item));
        args.append(',');
    }
    endList();
}

void DebuggerCommand::arg(const char *value)
{
    args.append("\"");
    args.append(value);
    args.append("\",");
}

void DebuggerCommand::beginList(const char *name)
{
    if (name) {
        args += '"';
        args += name;
        args += "\":";
    }
    args += '[';
}

void DebuggerCommand::endList()
{
    if (args.endsWith(','))
        args.chop(1);
    args += "],";
}

void DebuggerCommand::beginGroup(const char *name)
{
    if (name) {
        args += '"';
        args += name;
        args += "\":";
    }
    args += '{';
}

void DebuggerCommand::endGroup()
{
    if (args.endsWith(','))
        args.chop(1);
    args += "},";
}

QByteArray DebuggerCommand::arguments() const
{
    QByteArray result = args;
    if (result.endsWith(','))
        result.chop(1);
    return result;
}

} // namespace Internal
} // namespace Debugger
