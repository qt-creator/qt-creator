// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerprotocol.h"
#include "debuggertr.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QTimeZone>
#include <QJsonArray>
#include <QJsonDocument>

#include <utils/processhandle.h>
#include <utils/qtcassert.h>

namespace Debugger::Internal {

static uchar fromhex(uchar c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return 10 + c - 'a';
    if (c >= 'A' && c <= 'Z')
        return 10 + c - 'A';
    return UCHAR_MAX;
}

// DebuggerOutputParser

DebuggerOutputParser::DebuggerOutputParser(const QString &output)
    : from(output.begin()), to(output.end())
{
}

void DebuggerOutputParser::skipCommas()
{
    while (from < to && *from == ',')
        ++from;
}

void DebuggerOutputParser::skipSpaces()
{
    while (from < to && QChar::isSpace(from->unicode()))
        ++from;
}

QStringView DebuggerOutputParser::readString(const std::function<bool(char)> &isValidChar)
{
    const QChar *oldFrom = from;
    while (from < to && isValidChar(from->unicode()))
        ++from;
    return {oldFrom, from};
}

int DebuggerOutputParser::readInt()
{
    int res = 0;
    while (from < to && *from >= '0' && *from <= '9') {
        res *= 10;
        res += (*from++).unicode() - '0';
    }
    return res;
}

QChar DebuggerOutputParser::readChar()
{
    return *from++;
}

static bool isNameChar(char c)
{
    return c != '=' && c != ':' && c != ']' && !QChar::isSpace(c);
}

void GdbMi::parseResultOrValue(DebuggerOutputParser &parser)
{
    parser.skipSpaces();

    if (parser.isAtEnd())
        return;

    //qDebug() << "parseResultOrValue: " << parser.buffer();
    parseValue(parser);
    parser.skipSpaces();
    if (isValid()) {
        //qDebug() << "no valid result in " << parser.buffer();
        return;
    }
    if (parser.isAtEnd())
        return;
    if (parser.isCurrent('(')) {
        parser.advance();
        return;
    }

    m_name = parser.readString(isNameChar).toString();

    if (!parser.isAtEnd() && parser.isCurrent('=')) {
        parser.advance();
        parseValue(parser);
    }
}

// Reads one \ooo entity.
static bool parseOctalEscapedHelper(DebuggerOutputParser &parser, DebuggerOutputParser::Buffer &buffer)
{
    if (parser.remainingChars() < 4)
        return false;

    const char c1 = parser.lookAhead(1).unicode();
    const char c2 = parser.lookAhead(2).unicode();
    const char c3 = parser.lookAhead(3).unicode();
    if (!isdigit(c1) || !isdigit(c2) || !isdigit(c3))
        return false;

    buffer += char((c1 - '0') * 64 + (c2 - '0') * 8 + (c3 - '0'));
    parser.advance(4);
    return true;
}

static bool parseHexEscapedHelper(DebuggerOutputParser &parser, DebuggerOutputParser::Buffer &buffer)
{
    if (parser.remainingChars() < 4)
        return false;
    if (parser.lookAhead(1) != 'x')
        return false;

    const char c1 = parser.lookAhead(2).unicode();
    const char c2 = parser.lookAhead(3).unicode();
    if (!isxdigit(c1) || !isxdigit(c2))
        return false;

    buffer += char(16 * fromhex(c1) + fromhex(c2));
    parser.advance(4);
    return true;
}

static void parseSimpleEscape(DebuggerOutputParser &parser, DebuggerOutputParser::Buffer &buffer)
{
    if (parser.isAtEnd()) {
        qDebug() << "MI Parse Error, unterminated backslash escape";
        return;
    }

    const QChar c = parser.current();
    parser.advance();
    switch (c.unicode()) {
        case 'a': buffer += '\a'; break;
        case 'b': buffer += '\b'; break;
        case 'f': buffer += '\f'; break;
        case 'n': buffer += '\n'; break;
        case 'r': buffer += '\r'; break;
        case 't': buffer += '\t'; break;
        case 'v': buffer += '\v'; break;
        case '"': buffer += '"'; break;
        case '\'': buffer += '\''; break;
        case '\\': buffer += '\\'; break;
        default:
            qDebug() << "MI Parse Error, unrecognized backslash escape";
    }
}

// Reads one \123 or \x12 entity, *or* one escaped char, *or* one unescaped char.
static void parseCharOrEscape(DebuggerOutputParser &parser, DebuggerOutputParser::Buffer &buffer)
{
    if (parser.isCurrent('\\')) {
        if (parseOctalEscapedHelper(parser, buffer))
            return;
        if (parseHexEscapedHelper(parser, buffer))
            return;
        parser.advance();
        parseSimpleEscape(parser, buffer);
    } else {
        buffer += char(parser.readChar().unicode());
    }
}

void DebuggerOutputParser::readCStringData(Buffer &buffer)
{
    if (isAtEnd())
        return;

    if (*from != '"') {
        qDebug() << "MI Parse Error, double quote expected";
        ++from; // So we don't hang
        return;
    }

    ++from; // Skip initial quote.
    while (from < to) {
        if (*from == '"') {
            ++from;
            return;
        }
        parseCharOrEscape(*this, buffer);
    }

    qDebug() << "MI Parse Error, unfinished string";
}

QString DebuggerOutputParser::readCString()
{
    Buffer buffer;
    readCStringData(buffer);
    return QString::fromUtf8(buffer);
}

void GdbMi::parseValue(DebuggerOutputParser &parser)
{
    if (parser.isAtEnd())
        return;

    //qDebug() << "parseValue: " << parser;
    switch (parser.current().unicode()) {
        case '{':
            parseTuple(parser);
            break;
        case '[':
            parseList(parser);
            break;
        case '"':
            m_type = Const;
            m_data = parser.readCString();
            break;
        default:
            break;
    }
}

void GdbMi::parseTuple(DebuggerOutputParser &parser)
{
    //qDebug() << "parseTuple: " << parser.buffer();
    QTC_CHECK(parser.isCurrent('{'));
    parser.advance();
    parseTuple_helper(parser);
}

void GdbMi::parseTuple_helper(DebuggerOutputParser &parser)
{
    parser.skipCommas();
    //qDebug() << "parseTuple_helper: " << parser.buffer();
    m_type = Tuple;
    while (!parser.isAtEnd()) {
        if (parser.isCurrent('}')) {
            parser.advance();
            break;
        }
        GdbMi child;
        child.parseResultOrValue(parser);
        //qDebug() << "\n=======\n" << qPrintable(child.toString()) << "\n========\n";
        if (!child.isValid())
            return;
        m_children.push_back(child);
        parser.skipCommas();
    }
}

void GdbMi::parseList(DebuggerOutputParser &parser)
{
    //qDebug() << "parseList: " << parser.buffer();
    QTC_CHECK(parser.isCurrent('['));
    parser.advance();
    m_type = List;
    parser.skipCommas();
    while (true) {
        QTC_ASSERT(!parser.isAtEnd(), break);
        if (parser.isCurrent(']')) {
            parser.advance();
            break;
        }
        GdbMi child;
        child.parseResultOrValue(parser);
        if (!child.isValid())
            break;
        m_children.push_back(child);
        parser.skipCommas();
    }
}

static QString ind(int indent)
{
    return QString(2 * indent, QChar(' '));
}

void GdbMi::dumpChildren(QString * str, bool multiline, int indent) const
{
    bool first = true;
    for (const GdbMi &child : *this) {
        if (first) {
            first = false;
        } else {
            *str += ',';
            if (multiline)
                *str += '\n';
        }

        if (multiline)
            *str += ind(indent);
        *str += child.toString(multiline, indent);
    }
}

QString GdbMi::escapeCString(const QString &ba)
{
    QString ret;
    ret.reserve(ba.length() * 2);
    for (int i = 0; i < ba.length(); ++i) {
        const ushort c = ba.at(i).unicode();
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
                    ret += QLatin1Char('0' + (c >> 6));
                    ret += QLatin1Char('0' + ((c >> 3) & 7));
                    ret += QLatin1Char('0' + (c & 7));
                } else {
                    ret += c;
                }
        }
    }
    return ret;
}

QString GdbMi::toString(bool multiline, int indent) const
{
    QString result;
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

void GdbMi::fromString(const QString &ba)
{
    DebuggerOutputParser parser(ba);
    parseResultOrValue(parser);
}

void GdbMi::fromStringMultiple(const QString &ba)
{
    DebuggerOutputParser parser(ba);
    parseTuple_helper(parser);
}

const GdbMi &GdbMi::operator[](const char *name) const
{
    static GdbMi empty;
    for (const GdbMi &child : *this)
        if (child.m_name == QLatin1String(name))
            return child;
    return empty;
}

qulonglong GdbMi::toAddress() const
{
    QString ba = m_data;
    if (ba.endsWith('L'))
        ba.chop(1);
    if (ba.startsWith('*') || ba.startsWith('@'))
        ba = ba.mid(1);
    return ba.toULongLong(nullptr, 0);
}

Utils::ProcessHandle GdbMi::toProcessHandle() const
{
    return Utils::ProcessHandle(m_data.toULongLong());
}

//////////////////////////////////////////////////////////////////////////////////
//
// GdbResponse
//
//////////////////////////////////////////////////////////////////////////////////

QString DebuggerResponse::stringFromResultClass(ResultClass resultClass)
{
    switch (resultClass) {
        case ResultDone: return QLatin1String("done");
        case ResultRunning: return QLatin1String("running");
        case ResultConnected: return QLatin1String("connected");
        case ResultError: return QLatin1String("error");
        case ResultExit: return QLatin1String("exit");
        default: return QLatin1String("unknown");
    }
}

QString DebuggerResponse::toString() const
{
    QString result;
    if (token != -1)
        result = QString::number(token);
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

//! Extract the GDB version number from the output of 'gdb --version'.
//! \param[out] gdbVersion GDB version "hash" with major*10000 + minor*100 + patch
//!             e.g. version GDB 3.7.14 will set this to 30714
//! \param[out] gdbBuildVersion distribution dependent value
//! \note See the file tests/auto/debugger/tst_gdb.cpp for example conversions.
void extractGdbVersion(const QString &msg,
    int *gdbVersion, int *gdbBuildVersion, bool *isMacGdb, bool *isQnxGdb)
{
    const QChar dot('.');

    const bool ignoreParenthesisContent = msg.contains("rubenvb")
                                       || msg.contains("openSUSE")
                                       || msg.contains("SUSE Linux Enterprise");

    const QChar parOpen('(');
    const QChar parClose(')');

    QString cleaned;
    QString build;
    bool inClean = true;
    bool inParenthesis = false;

    int gdbMsgBegin = msg.indexOf("GNU gdb");
    if (gdbMsgBegin == -1)
      gdbMsgBegin = 0;

    for (int i = gdbMsgBegin, gdbMsgSize = msg.size(); i < gdbMsgSize; ++i) {
        QChar c = msg.at(i);
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

    *isMacGdb = msg.contains("Apple version");
    *isQnxGdb = msg.contains("qnx");

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

static QString quoteUnprintableLatin1(const QString &ba)
{
    QString res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const unsigned char c = ba.at(i).unicode();
        if (isprint(c)) {
            res += ba.at(i);
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
static void getDateTime(qint64 msecs, int status, QDate *date, QTime *time, int tiVersion)
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

    *date = ((status & NullDate) && tiVersion < 14) ? QDate() : QDate::fromJulianDay(jd);
    *time = ((status & NullTime) && tiVersion < 14) ? QTime() : QTime::fromMSecsSinceStartOfDay(ds);
}

QString decodeData(const QString &ba, const QString &encoding)
{
    if (encoding.isEmpty())
        return quoteUnprintableLatin1(ba); // The common case.

    if (encoding == "empty")
        return Tr::tr("<empty>");
    if (encoding == "minimumitemcount")
        return Tr::tr("<at least %n items>", nullptr, ba.toInt());
    if (encoding == "undefined")
        return QLatin1String("Undefined");
    if (encoding == "null")
        return QLatin1String("Null");
    if (encoding == "itemcount")
        return Tr::tr("<%n items>", nullptr, ba.toInt());
    if (encoding == "notaccessible")
        return Tr::tr("<not accessible>");
    if (encoding == "optimizedout")
        return Tr::tr("<optimized out>");
    if (encoding == "nullreference")
        return Tr::tr("<null reference>");
    if (encoding == "emptystructure")
        return QLatin1String("{...}");
    if (encoding == "uninitialized")
        return Tr::tr("<uninitialized>");
    if (encoding == "invalid")
        return Tr::tr("<invalid>");
    if (encoding == "notcallable")
        return Tr::tr("<not callable>");
    if (encoding == "outofscope")
        return Tr::tr("<out of scope>");

    DebuggerEncoding enc(encoding);
    QString result;
    switch (enc.type) {
        case DebuggerEncoding::Unencoded: {
            result = quoteUnprintableLatin1(ba);
            break;
        }
        case DebuggerEncoding::HexEncodedLocal8Bit: {
            const QByteArray decodedBa = QByteArray::fromHex(ba.toUtf8());
            result = QString::fromLocal8Bit(decodedBa.data(), decodedBa.size());
            break;
        }
        case DebuggerEncoding::HexEncodedLatin1: {
            const QByteArray decodedBa = QByteArray::fromHex(ba.toUtf8());
            result = QString::fromLatin1(decodedBa.data(), decodedBa.size());
            break;
        }
        case DebuggerEncoding::HexEncodedUtf8: {
            const QByteArray decodedBa = QByteArray::fromHex(ba.toUtf8());
            result = QString::fromUtf8(decodedBa.data(), decodedBa.size());
            break;
        }
        case DebuggerEncoding::HexEncodedUtf16: {
            const QByteArray decodedBa = QByteArray::fromHex(ba.toUtf8());
            result = QString::fromUtf16(reinterpret_cast<const char16_t *>
                (decodedBa.data()), decodedBa.size() / 2);
            break;
        }
        case DebuggerEncoding::HexEncodedUcs4: {
            const QByteArray decodedBa = QByteArray::fromHex(ba.toUtf8());
            result = QString::fromUcs4(reinterpret_cast<const char32_t *>
                (decodedBa.data()), decodedBa.size() / 4);
            break;
        }
        case DebuggerEncoding::JulianDate: {
            const QDate date = dateFromData(ba.toInt());
            return date.isValid() ? date.toString(Qt::TextDate) : "(invalid)";
        }
        case DebuggerEncoding::MillisecondsSinceMidnight: {
            const QTime time = timeFromData(ba.toInt());
            return time.isValid() ? time.toString(Qt::TextDate) : "(invalid)";
        }
        case DebuggerEncoding::JulianDateAndMillisecondsSinceMidnight: {
            const int p = ba.indexOf('/');
            const QDate date = dateFromData(ba.left(p).toInt());
            const QTime time = timeFromData(ba.mid(p + 1).toInt());
            const QDateTime dateTime = QDateTime(date, time);
            return dateTime.isValid() ? dateTime.toString(Qt::TextDate) : "(invalid)";
        }
        case DebuggerEncoding::HexEncodedUnsignedInteger:
        case DebuggerEncoding::HexEncodedSignedInteger:
            qDebug("not implemented"); // Only used in Arrays, see watchdata.cpp
            return QString();
        case DebuggerEncoding::HexEncodedFloat: {
            QByteArray s = QByteArray::fromHex(ba.toUtf8());
            if (s.size() < enc.size)
                s.prepend(QByteArray(enc.size - s.size(), '\0'));
            if (enc.size == 4) {
                union { char c[4]; float f; } u = {{s[3], s[2], s[1], s[0]}};
                return QString::number(u.f);
            }
            if (enc.size == 8) {
                union { char c[8]; double d; } u = {{s[7], s[6], s[5], s[4], s[3], s[2], s[1], s[0]}};
                return QString::number(u.d);
            }
            break;
        }
        case DebuggerEncoding::IPv6AddressAndHexScopeId: { // 16 hex-encoded bytes, "%" and the string-encoded scope
            const int p = ba.indexOf('%');
            QHostAddress ip6(p == -1 ? ba : ba.left(p));
            if (ip6.isNull())
                break;

            const QByteArray scopeId = p == -1 ? QByteArray() : QByteArray::fromHex(ba.mid(p + 1).toUtf8());
            if (!scopeId.isEmpty())
                ip6.setScopeId(QString::fromUtf16(reinterpret_cast<const char16_t *>(scopeId.constData()),
                                                  scopeId.length() / 2));
            return ip6.toString();
        }
        case DebuggerEncoding::DateTimeInternal: { // DateTimeInternal: msecs, spec, offset, tz, status
            int p0 = ba.indexOf('/');
            int p1 = ba.indexOf('/', p0 + 1);
            int p2 = ba.indexOf('/', p1 + 1);
            int p3 = ba.indexOf('/', p2 + 1);
            int p4 = ba.indexOf('/', p3 + 1);

            qint64 msecs = ba.left(p0).toLongLong();
            ++p0;
            Qt::TimeSpec spec = Qt::TimeSpec(ba.mid(p0, p1 - p0).toInt());
            ++p1;
            qulonglong offset = ba.mid(p1, p2 - p1).toInt();
            ++p2;
            QByteArray timeZoneId = QByteArray::fromHex(ba.mid(p2, p3 - p2).toUtf8());
            ++p3;
            int status = ba.mid(p3, p4 - p3).toInt();
            ++p4;
            int tiVersion = ba.mid(p4).toInt();

            QDate date;
            QTime time;
            getDateTime(msecs, status, &date, &time, tiVersion);

            QDateTime dateTime;
            if (spec == Qt::OffsetFromUTC) {
                dateTime = QDateTime(date, time, spec, offset);
            } else if (spec == Qt::TimeZone) {
                QTimeZone tz(timeZoneId);
                if (!tz.isValid())
                    return QLatin1String("<unavailable>");

                dateTime = QDateTime(date, time, tz);
            } else {
                dateTime = QDateTime(date, time, spec);
            }
            return dateTime.toString();
        }
        qDebug() << "ENCODING ERROR: " << enc.type;
        return Tr::tr("<Encoding error>");
    }

    if (enc.quotes) {
        const QChar doubleQuote('"');
        result = doubleQuote + result + doubleQuote;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////
//
// DebuggerCommand
//
//////////////////////////////////////////////////////////////////////////////////

template<typename Value>
QJsonValue addToJsonObject(const QJsonValue &args, const char *name, const Value &value)
{
    QTC_ASSERT(args.isObject() || args.isNull(), return args);
    QJsonObject obj = args.toObject();
    obj.insert(QLatin1String(name), value);
    return obj;
}

void DebuggerCommand::arg(const char *name, int value)
{
    args = addToJsonObject(args, name, value);
}

void DebuggerCommand::arg(const char *name, qlonglong value)
{
    args = addToJsonObject(args, name, value);
}

void DebuggerCommand::arg(const char *name, qulonglong value)
{
    // gdb and lldb will correctly cast the value back to unsigned if needed, so this is no problem.
    args = addToJsonObject(args, name, qint64(value));
}

void DebuggerCommand::arg(const char *name, const QString &value)
{
    args = addToJsonObject(args, name, value);
}

void DebuggerCommand::arg(const char *name, const char *value)
{
    args = addToJsonObject(args, name, value);
}

void DebuggerCommand::arg(const char *name, const QList<int> &list)
{
    QJsonArray numbers;
    for (int item : list)
        numbers.append(item);
    args = addToJsonObject(args, name, numbers);
}

void DebuggerCommand::arg(const char *name, const QStringList &list)
{
    QJsonArray arr;
    for (const QString &item : list)
        arr.append(toHex(item));
    args = addToJsonObject(args, name, arr);
}

void DebuggerCommand::arg(const char *value)
{
    QTC_ASSERT(args.isArray() || args.isNull(), return);
    QJsonArray arr = args.toArray();
    arr.append(value);
    args = arr;
}

void DebuggerCommand::arg(const char *name, bool value)
{
    args = addToJsonObject(args, name, value);
}

void DebuggerCommand::arg(const char *name, const QJsonValue &value)
{
    args = addToJsonObject(args, name, value);
}

static QJsonValue translateJsonToPython(const QJsonValue &value)
{
    // TODO: Verify that this covers all incompatibilities between python and json,
    //       e.g. number format and precision

    switch (value.type()) {
    // Undefined is not a problem as the JSON generator ignores that.
    case QJsonValue::Null:
        // Python doesn't understand "null"
        return QJsonValue(0);
    case QJsonValue::Bool:
        // Python doesn't understand lowercase "true" or "false"
        return QJsonValue(value.toBool() ? 1 : 0);
    case QJsonValue::Object: {
        QJsonObject object = value.toObject();
        for (QJsonObject::iterator i = object.begin(); i != object.end(); ++i)
            i.value() = translateJsonToPython(i.value());
        return object;
    }
    case QJsonValue::Array: {
        QJsonArray array = value.toArray();
        for (QJsonArray::iterator i = array.begin(); i != array.end(); ++i)
            *i = translateJsonToPython(*i);
        return array;
    }
    default:
        return value;
    }
}

QString DebuggerCommand::argsToPython() const
{
    QJsonValue pythonCompatible(translateJsonToPython(args));
    if (pythonCompatible.isArray())
        return QString::fromUtf8(QJsonDocument(pythonCompatible.toArray()).toJson(QJsonDocument::Compact));
    else
        return QString::fromUtf8(QJsonDocument(pythonCompatible.toObject()).toJson(QJsonDocument::Compact));
}

QString DebuggerCommand::argsToString() const
{
    return args.toString();
}

DebuggerEncoding::DebuggerEncoding(const QString &data)
{
    const QStringList l = data.split(':');

    const QString &t = l.at(0);
    if (t == "latin1") {
        type = HexEncodedLatin1;
        size = 1;
        quotes = true;
    } else if (t == "local8bit") {
        type = HexEncodedLocal8Bit;
        size = 1;
        quotes = true;
    } else if (t == "utf8") {
        type = HexEncodedUtf8;
        size = 1;
        quotes = true;
    } else if (t == "utf16") {
        type = HexEncodedUtf16;
        size = 2;
        quotes = true;
    } else if (t == "ucs4") {
        type = HexEncodedUcs4;
        size = 4;
        quotes = true;
    } else if (t == "int") {
        type = HexEncodedSignedInteger;
    } else if (t == "uint") {
        type = HexEncodedUnsignedInteger;
    } else if (t == "float") {
        type = HexEncodedFloat;
    } else if (t == "juliandate") {
        type = JulianDate;
    } else if (t == "juliandateandmillisecondssincemidnight") {
        type = JulianDateAndMillisecondsSinceMidnight;
    } else if (t == "millisecondssincemidnight") {
        type = MillisecondsSinceMidnight;
    } else if (t == "ipv6addressandhexscopeid") {
        type = IPv6AddressAndHexScopeId;
    } else if (t == "datetimeinternal") {
        type = DateTimeInternal;
    } else if (!t.isEmpty()) {
        qDebug() << "CANNOT DECODE ENCODING" << data;
    }

    if (l.size() >= 2)
        size = l.at(1).toInt();

    if (l.size() >= 3)
        quotes = bool(l.at(2).toInt());
}

QString DebuggerEncoding::toString() const
{
    return QString("%1:%2:%3").arg(type).arg(size).arg(quotes);
}

QString fromHex(const QString &str)
{
    return QString::fromUtf8(QByteArray::fromHex(str.toUtf8()));
}

QString toHex(const QString &str)
{
    return QString::fromUtf8(str.toUtf8().toHex());
}

} // Debugger::Internal
