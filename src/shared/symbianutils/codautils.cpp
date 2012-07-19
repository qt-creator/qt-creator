/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "codautils.h"
#include <ctype.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDate>
#include <QDateTime>
#include <QTime>

#define logMessage(s)  do { qDebug() << "CODACLIENT: " << s; } while (0)

namespace Coda {

Library::Library() : codeseg(0), dataseg(0), pid(0)
{
}

Library::Library(const CodaResult &result) : codeseg(0), dataseg(0), pid(0)
{
    if (result.data.size() < 20) {
        qWarning("Invalid CODA creation notification received.");
        return;
    }

    const char *data = result.data.constData();
    pid = extractInt(data + 2);
    codeseg = extractInt(data + 10);
    dataseg = extractInt(data + 14);
    const uint len = extractShort(data + 18);
    name = result.data.mid(20, len);
}

CodaAppVersion::CodaAppVersion()
{
    reset();
}

void CodaAppVersion::reset()
{
    codaMajor = codaMinor = protocolMajor = protocolMinor = 0;
}

Session::Session()
{
    reset();
}

void Session::reset()
{
    cpuMajor = 0;
    cpuMinor = 0;
    bigEndian = 0;
    defaultTypeSize = 0;
    fpTypeSize = 0;
    extended1TypeSize = 0;
    extended2TypeSize = 0;
    pid = 0;
    mainTid = 0;
    tid = 0;
    codeseg = 0;
    dataseg = 0;

    libraries.clear();
    codaAppVersion.reset();
}

static QString formatCpu(int major, int minor)
{
    //: CPU description of an S60 device
    //: %1 major verison, %2 minor version
    //: %3 real name of major verison, %4 real name of minor version
    const QString str = QCoreApplication::translate("Coda::Session", "CPU: v%1.%2%3%4");
    QString majorStr;
    QString minorStr;
    switch (major) {
    case 0x04:
        majorStr = " ARM";
        break;
    }
    switch (minor) {
    case 0x00:
        minorStr = " 920T";
        break;
    }
    return str.arg(major).arg(minor).arg(majorStr).arg(minorStr);
 }

QString formatCodaVersion(const CodaAppVersion &version)
{
    QString str = QCoreApplication::translate("Coda::Session",
                                              "CODA: v%1.%2 CODA protocol: v%3.%4");
    str = str.arg(version.codaMajor).arg(version.codaMinor);
    return str.arg(version.protocolMajor).arg(version.protocolMinor);
}

QString Session::deviceDescription(unsigned verbose) const
{
    if (!cpuMajor)
        return QString();

    //: s60description
    //: description of an S60 device
    //: %1 CPU description, %2 endianness
    //: %3 default type size (if any), %4 float size (if any)
    //: %5 Coda version
    QString msg = QCoreApplication::translate("Coda::Session", "%1, %2%3%4, %5");
    QString endianness = bigEndian
                         ? QCoreApplication::translate("Coda::Session", "big endian")
                         : QCoreApplication::translate("Coda::Session", "little endian");
    msg = msg.arg(formatCpu(cpuMajor, cpuMinor)).arg(endianness);
    QString defaultTypeSizeStr;
    QString fpTypeSizeStr;
    if (verbose && defaultTypeSize)
        //: will be inserted into s60description
        defaultTypeSizeStr = QCoreApplication::translate("Coda::Session", ", type size: %1").arg(defaultTypeSize);
    if (verbose && fpTypeSize)
        //: will be inserted into s60description
        fpTypeSizeStr = QCoreApplication::translate("Coda::Session", ", float size: %1").arg(fpTypeSize);
    msg = msg.arg(defaultTypeSizeStr).arg(fpTypeSizeStr);
    return msg.arg(formatCodaVersion(codaAppVersion));
}

QByteArray Session::gdbLibraryList() const
{
    const int count = libraries.size();
    QByteArray response = "l<library-list>";
    for (int i = 0; i != count; ++i) {
        const Coda::Library &lib = libraries.at(i);
        response += "<library name=\"";
        response += lib.name;
        response += "\">";
        response += "<section address=\"0x";
        response += Coda::hexNumber(lib.codeseg);
        response += "\"/>";
        response += "<section address=\"0x";
        response += Coda::hexNumber(lib.dataseg);
        response += "\"/>";
        response += "<section address=\"0x";
        response += Coda::hexNumber(lib.dataseg);
        response += "\"/>";
        response += "</library>";
    }
    response += "</library-list>";
    return response;
}

QByteArray Session::gdbQsDllInfo(int start, int count) const
{
    // Happens with  gdb 6.4.50.20060226-cvs / CodeSourcery.
    // Never made it into FSF gdb that got qXfer:libraries:read instead.
    // http://sourceware.org/ml/gdb/2007-05/msg00038.html
    // Name=hexname,TextSeg=textaddr[,DataSeg=dataaddr]
    const int libraryCount = libraries.size();
    const int end = count < 0 ? libraryCount : qMin(libraryCount, start + count);
    QByteArray response(1, end == libraryCount ? 'l' : 'm');
    for (int i = start; i < end; ++i) {
        if (i != start)
            response += ';';
        const Library &lib = libraries.at(i);
        response += "Name=";
        response += lib.name.toHex();
        response += ",TextSeg=";
        response += hexNumber(lib.codeseg);
        response += ",DataSeg=";
        response += hexNumber(lib.dataseg);
    }
    return response;
}

QString Session::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str << "Session: " << deviceDescription(false) << '\n'
            << "pid: " << pid <<  "main thread: " << mainTid
            << " current thread: " << tid << ' ';
    str.setIntegerBase(16);
    str << " code: 0x" << codeseg << " data: 0x" << dataseg << '\n';
    if (const int libCount = libraries.size()) {
        str << "Libraries:\n";
        for (int i = 0; i < libCount; i++)
            str << " #" << i << ' ' << libraries.at(i).name
                << " code: 0x" << libraries.at(i).codeseg
                << " data: 0x" << libraries.at(i).dataseg << '\n';
    }
    if (const int moduleCount = modules.size()) {
        str << "Modules:\n";
        for (int i = 0; i < moduleCount; i++)
            str << " #" << i << ' ' << modules.at(i) << '\n';
    }
    str.setIntegerBase(10);
    if (!addressToBP.isEmpty()) {
        typedef QHash<uint, uint>::const_iterator BP_ConstIterator;
        str << "Breakpoints:\n";
        const BP_ConstIterator cend = addressToBP.constEnd();
        for (BP_ConstIterator it = addressToBP.constBegin(); it != cend; ++it) {
            str.setIntegerBase(16);
            str << "  0x" << it.key();
            str.setIntegerBase(10);
            str << ' ' << it.value() << '\n';
        }
    }

    return rc;
}

// --------------

QByteArray decode7d(const QByteArray &ba)
{
    QByteArray res;
    res.reserve(ba.size());
    for (int i = 0; i < ba.size(); ++i) {
        byte c = byte(ba.at(i));
        if (c == 0x7d) {
            ++i;
            c = 0x20 ^ byte(ba.at(i));
        }
        res.append(c);
    }
    return res;
}

QByteArray encode7d(const QByteArray &ba)
{
    QByteArray res;
    res.reserve(ba.size() + 2);
    for (int i = 0; i < ba.size(); ++i) {
        byte c = byte(ba.at(i));
        if (c == 0x7e || c == 0x7d) {
            res.append(0x7d);
            res.append(0x20 ^ c);
        } else {
            res.append(c);
        }
    }
    return res;
}

// FIXME: Use the QByteArray based version below?
static inline QString stringFromByte(byte c)
{
    return QString::fromLatin1("%1").arg(c, 2, 16, QChar('0'));
}

SYMBIANUTILS_EXPORT QString stringFromArray(const QByteArray &ba, int maxLen)
{
    QString str;
    QString ascii;
    const int size = maxLen == -1 ? ba.size() : qMin(ba.size(), maxLen);
    for (int i = 0; i < size; ++i) {
        const int c = byte(ba.at(i));
        str += QString::fromAscii("%1 ").arg(c, 2, 16, QChar('0'));
        ascii += QChar(c).isPrint() ? QChar(c) : QChar('.');
    }
    if (size != ba.size()) {
        str += QLatin1String("...");
        ascii += QLatin1String("...");
    }
    return str + QLatin1String("  ") + ascii;
}

SYMBIANUTILS_EXPORT QByteArray hexNumber(uint n, int digits)
{
    QByteArray ba = QByteArray::number(n, 16);
    if (digits == 0 || ba.size() == digits)
        return ba;
    return QByteArray(digits - ba.size(), '0') + ba;
}

SYMBIANUTILS_EXPORT QByteArray hexxNumber(uint n, int digits)
{
    return "0x" + hexNumber(n, digits);
}

CodaResult::CodaResult() :
    code(0),
    token(0),
    isDebugOutput(false)
{
}

void CodaResult::clear()
{
    code = token= 0;
    isDebugOutput = false;
    data.clear();
    cookie = QVariant();
}

QString CodaResult::toString() const
{
    QString res = stringFromByte(code);
    res += QLatin1String(" [");
    res += stringFromByte(token);
    res += QLatin1Char(']');
    res += QLatin1Char(' ');
    res += stringFromArray(data);
    return res;
}

QByteArray frameMessage(byte command, byte token, const QByteArray &data, bool serialFrame)
{
    byte s = command + token;
    for (int i = 0; i != data.size(); ++i)
        s += data.at(i);
    byte checksum = 255 - (s & 0xff);
    //int x = s + ~s;
    //logMessage("check: " << s << checksum << x;

    QByteArray response;
    response.reserve(data.size() + 3);
    response.append(char(command));
    response.append(char(token));
    response.append(data);
    response.append(char(checksum));

    QByteArray encodedData = encode7d(response);

    QByteArray ba;
    ba.reserve(encodedData.size() + 6);
    if (serialFrame) {
        ba.append(char(0x01));
        ba.append(char(0x90));
        const ushort encodedSize = encodedData.size() + 2; // 2 x 0x7e
        appendShort(&ba, encodedSize, BigEndian);
    }
    ba.append(char(0x7e));
    ba.append(encodedData);
    ba.append(char(0x7e));

    return ba;
}

/* returns 0 if array doesn't represent a result,
otherwise returns the length of the result data */
ushort isValidCodaResult(const QByteArray &buffer, bool serialFrame, ushort& mux)
{
    if (serialFrame) {
        // Serial protocol with length info
        if (buffer.length() < 4)
            return 0;
        mux = extractShort(buffer.data());
        const ushort len = extractShort(buffer.data() + 2);
        return (buffer.size() >= len + 4) ? len : ushort(0);
    }
    // Frameless protocol without length info
    const char delimiter = char(0x7e);
    const int firstDelimiterPos = buffer.indexOf(delimiter);
    // Regular message delimited by 0x7e..0x7e
    if (firstDelimiterPos == 0) {
        mux = MuxCoda;
        const int endPos = buffer.indexOf(delimiter, firstDelimiterPos + 1);
        return endPos != -1 ? endPos + 1 - firstDelimiterPos : 0;
    }
    // Some ASCII log message up to first delimiter or all
    return firstDelimiterPos != -1 ? firstDelimiterPos : buffer.size();
}

bool extractResult(QByteArray *buffer, bool serialFrame, CodaResult *result, bool &linkEstablishmentMode, QByteArray *rawData)
{
    result->clear();
    if(rawData)
        rawData->clear();
    ushort len = isValidCodaResult(*buffer, serialFrame, result->multiplex);
    // handle receiving application output, which is not a regular command
    const int delimiterPos = serialFrame ? 4 : 0;
    if (linkEstablishmentMode) {
        //when "hot connecting" a device, we can receive partial frames.
        //this code resyncs by discarding data until a CODA frame is found
        while (buffer->length() > delimiterPos
               && result->multiplex != MuxTextTrace
               && !(result->multiplex == MuxCoda && buffer->at(delimiterPos) == 0x7e)) {
            buffer->remove(0,1);
            len = isValidCodaResult(*buffer, serialFrame, result->multiplex);
        }
    }
    if (!len)
        return false;
    if (buffer->at(delimiterPos) != 0x7e) {
        result->isDebugOutput = true;
        result->data = buffer->mid(delimiterPos, len);
        buffer->remove(0, delimiterPos + len);
        return true;
    }
    // FIXME: what happens if the length contains 0xfe?
    // Assume for now that it passes unencoded!
    const QByteArray data = decode7d(buffer->mid(delimiterPos + 1, len - 2));
    if(rawData)
        *rawData = data;
    buffer->remove(0, delimiterPos + len);

    byte sum = 0;
    for (int i = 0; i < data.size(); ++i) // 3 = 2 * 0xfe + sum
        sum += byte(data.at(i));
    if (sum != 0xff)
        logMessage("*** CHECKSUM ERROR: " << byte(sum));

    result->code = data.at(0);
    result->token = data.at(1);
    result->data = data.mid(2, data.size() - 3);
    //logMessage("   REST BUF: " << stringFromArray(*buffer));
    //logMessage("   CURR DATA: " << stringFromArray(data));
    //QByteArray prefix = "READ BUF:                                       ";
    //logMessage((prefix + "HEADER: " + stringFromArray(header).toLatin1()).data());
    linkEstablishmentMode = false; //have received a good CODA packet, therefore in sync
    return true;
}

SYMBIANUTILS_EXPORT ushort extractShort(const char *data)
{
    return byte(data[0]) * 256 + byte(data[1]);
}

SYMBIANUTILS_EXPORT uint extractInt(const char *data)
{
    uint res = byte(data[0]);
    res *= 256; res += byte(data[1]);
    res *= 256; res += byte(data[2]);
    res *= 256; res += byte(data[3]);
    return res;
}

SYMBIANUTILS_EXPORT quint64 extractInt64(const char *data)
{
    quint64 res = byte(data[0]);
    res <<= 8; res += byte(data[1]);
    res <<= 8; res += byte(data[2]);
    res <<= 8; res += byte(data[3]);
    res <<= 8; res += byte(data[4]);
    res <<= 8; res += byte(data[5]);
    res <<= 8; res += byte(data[6]);
    res <<= 8; res += byte(data[7]);
    return res;
}

SYMBIANUTILS_EXPORT QString quoteUnprintableLatin1(const QByteArray &ba)
{
    QString res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const byte c = ba.at(i);
        if (isprint(c)) {
            res += c;
        } else {
            qsnprintf(buf, sizeof(buf) - 1, "\\%x", int(c));
            res += buf;
        }
    }
    return res;
}

SYMBIANUTILS_EXPORT void appendShort(QByteArray *ba, ushort s, Endianness endian)
{
    if (endian == BigEndian) {
        ba->append(s / 256);
        ba->append(s % 256);
    } else {
        ba->append(s % 256);
        ba->append(s / 256);
    }
}

SYMBIANUTILS_EXPORT void appendInt(QByteArray *ba, uint i, Endianness endian)
{
    const uchar b3 = i % 256; i /= 256;
    const uchar b2 = i % 256; i /= 256;
    const uchar b1 = i % 256; i /= 256;
    const uchar b0 = i;
    ba->reserve(ba->size() + 4);
    if (endian == BigEndian) {
        ba->append(b0);
        ba->append(b1);
        ba->append(b2);
        ba->append(b3);
    } else {
        ba->append(b3);
        ba->append(b2);
        ba->append(b1);
        ba->append(b0);
    }
}

void appendString(QByteArray *ba, const QByteArray &str, Endianness endian, bool appendNullTerminator)
{
    const int fullSize = str.size() + (appendNullTerminator ? 1 : 0);
    appendShort(ba, fullSize, endian); // count the terminating \0
    ba->append(str);
    if (appendNullTerminator)
        ba->append('\0');
}

void appendDateTime(QByteArray *ba, QDateTime dateTime, Endianness endian)
{
    // convert the QDateTime to UTC and append its representation to QByteArray
    // format is the same as in FAT file system
    dateTime = dateTime.toUTC();
    const QTime utcTime = dateTime.time();
    const QDate utcDate = dateTime.date();
    uint fatDateTime = (utcTime.hour() << 11 | utcTime.minute() << 5 | utcTime.second()/2) << 16;
    fatDateTime |= (utcDate.year()-1980) << 9 | utcDate.month() << 5 | utcDate.day();
    appendInt(ba, fatDateTime, endian);
}

QByteArray errorMessage(byte code)
{
    switch (code) {
        case 0x00: return "No error";
        case 0x01: return "Generic error in CWDS message";
        case 0x02: return "Unexpected packet size in send msg";
        case 0x03: return "Internal error occurred in CWDS";
        case 0x04: return "Escape followed by frame flag";
        case 0x05: return "Bad FCS in packet";
        case 0x06: return "Packet too long";
        case 0x07: return "Sequence ID not expected (gap in sequence)";

        case 0x10: return "Command not supported";
        case 0x11: return "Command param out of range";
        case 0x12: return "An option was not supported";
        case 0x13: return "Read/write to invalid memory";
        case 0x14: return "Read/write invalid registers";
        case 0x15: return "Exception occurred in CWDS";
        case 0x16: return "Targeted system or thread is running";
        case 0x17: return "Breakpoint resources (HW or SW) exhausted";
        case 0x18: return "Requested breakpoint conflicts with existing one";

        case 0x20: return "General OS-related error";
        case 0x21: return "Request specified invalid process";
        case 0x22: return "Request specified invalid thread";
    }
    return "Unknown error";
}

uint swapEndian(uint in)
{
    return (in>>24) | ((in<<8) & 0x00FF0000) | ((in>>8) & 0x0000FF00) | (in<<24);
}

int CodaResult::errorCode() const
{
    // NAK means always error, else data sized 1 with a non-null element
    const bool isNAK = code == 0xff;
    if (data.size() != 1 && !isNAK)
        return 0;
    if (const int errorCode = data.at(0))
        return errorCode;
    return isNAK ? 0xff : 0;
}

QString CodaResult::errorString() const
{
    // NAK means always error, else data sized 1 with a non-null element
    if (code == 0xff)
        return "NAK";
    if (data.size() < 1)
        return "Unknown error packet";
    return errorMessage(data.at(0));
}

} // namespace Coda

