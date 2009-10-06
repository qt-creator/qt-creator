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

#include "trkutils.h"
#include <ctype.h>

#include <QtCore/QDebug>

#define logMessage(s)  do { qDebug() << "TRKCLIENT: " << s; } while (0)

namespace trk {

// FIXME: Use the QByteArray based version below?
QString stringFromByte(byte c)
{
    return QString("%1 ").arg(c, 2, 16, QChar('0'));
}

QString stringFromArray(const QByteArray &ba, int maxLen)
{
    QString str;
    QString ascii;
    const int size = maxLen == -1 ? ba.size() : qMin(ba.size(), maxLen);
    for (int i = 0; i < size; ++i) {
        //if (i == 5 || i == ba.size() - 2)
        //    str += "  ";
        int c = byte(ba.at(i));
        str += QString("%1 ").arg(c, 2, 16, QChar('0'));
        if (i >= 8 && i < ba.size() - 2)
            ascii += QChar(c).isPrint() ? QChar(c) : QChar('.');
    }
    if (size != ba.size()) {
        str += "...";
        ascii += "...";
    }
    return str + "  " + ascii;
}

QByteArray hexNumber(uint n, int digits)
{
    QByteArray ba = QByteArray::number(n, 16);
    if (digits == 0 || ba.size() == digits)
        return ba;
    return QByteArray(digits - ba.size(), '0') + ba;
}

QByteArray hexxNumber(uint n, int digits)
{
    return "0x" + hexNumber(n, digits);
}

TrkResult::TrkResult() :
    code(0),
    token(0),
    isDebugOutput(false)
{
}

void TrkResult::clear()
{
    code = token= 0;
    isDebugOutput = false;
    data.clear();
    cookie = QVariant();
}

QString TrkResult::toString() const
{
    QString res = stringFromByte(code) + "[" + stringFromByte(token);
    res.chop(1);
    return res + "] " + stringFromArray(data);
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
    response.append(char(command));
    response.append(char(token));
    response.append(data);
    response.append(char(checksum));

    QByteArray encodedData = encode7d(response);

    QByteArray ba;
    if (serialFrame) {
        ba.append(char(0x01));
        ba.append(char(0x90));
        const ushort encodedSize = encodedData.size() + 2; // 2 x 0x7e
        ba.append(char(encodedSize / 256));
        ba.append(char(encodedSize % 256));
    }
    ba.append(char(0x7e));
    ba.append(encodedData);
    ba.append(char(0x7e));

    return ba;
}

/* returns 0 if array doesn't represent a result,
otherwise returns the length of the result data */
ushort isValidTrkResult(const QByteArray &buffer, bool serialFrame)
{
    if (serialFrame) {
        // Serial protocol with length info
        if (buffer.length() < 4)
            return 0;
        if (buffer.at(0) != 0x01 || byte(buffer.at(1)) != 0x90)
            return 0;
        const ushort len = extractShort(buffer.data() + 2);
        return (buffer.size() >= len + 4) ? len : ushort(0);
    }
    // Frameless protocol without length info
    const char delimiter = char(0x7e);
    const int firstDelimiterPos = buffer.indexOf(delimiter);
    // Regular message delimited by 0x7e..0x7e
    if (firstDelimiterPos == 0) {
        const int endPos = buffer.indexOf(delimiter, firstDelimiterPos + 1);
        return endPos != -1 ? endPos + 1 - firstDelimiterPos : 0;
    }
    // Some ASCII log message up to first delimiter or all
    return firstDelimiterPos != -1 ? firstDelimiterPos : buffer.size();
}

bool extractResult(QByteArray *buffer, bool serialFrame, TrkResult *result, QByteArray *rawData)
{
    result->clear();
    if(rawData)
        rawData->clear();
    const ushort len = isValidTrkResult(*buffer, serialFrame);
    if (!len)
        return false;
    // handle receiving application output, which is not a regular command
    const int delimiterPos = serialFrame ? 4 : 0;
    if (buffer->at(delimiterPos) != 0x7e) {
        result->isDebugOutput = true;
        result->data = buffer->mid(delimiterPos, len);
        result->data.replace("\r\n", "\n");
        *buffer->remove(0, delimiterPos + len);
        return true;
    }
    // FIXME: what happens if the length contains 0xfe?
    // Assume for now that it passes unencoded!
    const QByteArray data = decode7d(buffer->mid(delimiterPos + 1, len - 2));
    if(rawData)
        *rawData = data;
    *buffer->remove(0, delimiterPos + len);

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
    return true;
}

ushort extractShort(const char *data)
{
    return byte(data[0]) * 256 + byte(data[1]);
}

uint extractInt(const char *data)
{
    uint res = byte(data[0]);
    res *= 256; res += byte(data[1]);
    res *= 256; res += byte(data[2]);
    res *= 256; res += byte(data[3]);
    return res;
}

QString quoteUnprintableLatin1(const QByteArray &ba)
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

QByteArray decode7d(const QByteArray &ba)
{
    QByteArray res;
    res.reserve(ba.size() + 2);
    for (int i = 0; i < ba.size(); ++i) {
        byte c = byte(ba.at(i));
        if (c == 0x7d) {
            ++i;
            c = 0x20 ^ byte(ba.at(i));
        }
        res.append(c);
    }
    //if (res != ba)
    //    logMessage("DECODED: " << stringFromArray(ba)
    //        << " -> " << stringFromArray(res));
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
    //if (res != ba)
    //    logMessage("ENCODED: " << stringFromArray(ba)
    //        << " -> " << stringFromArray(res));
    return res;
}

void appendByte(QByteArray *ba, byte b)
{
    ba->append(b);
}

void appendShort(QByteArray *ba, ushort s, Endianness endian)
{
    if (endian == BigEndian) {
        ba->append(s / 256);
        ba->append(s % 256);
    } else {
        ba->append(s % 256);
        ba->append(s / 256);
    }
}

void appendInt(QByteArray *ba, uint i, Endianness endian)
{
    int b3 = i % 256; i -= b3; i /= 256;
    int b2 = i % 256; i -= b2; i /= 256;
    int b1 = i % 256; i -= b1; i /= 256;
    int b0 = i % 256; i -= b0; i /= 256;
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
    const int n = str.size();
    const int fullSize = n + (appendNullTerminator ? 1 : 0);
    appendShort(ba, fullSize, endian); // count the terminating \0
    for (int i = 0; i != n; ++i)
        ba->append(str.at(i));
    if (appendNullTerminator)
        ba->append('\0');
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

int TrkResult::errorCode() const
{
    // NAK means always error, else data sized 1 with a non-null element
    const bool isNAK = code == 0xff;
    if (data.size() != 1 && !isNAK)
        return 0;
    if (const int errorCode = data.at(0))
        return errorCode;
    return isNAK ? 0xff : 0;
}

QString TrkResult::errorString() const
{
    // NAK means always error, else data sized 1 with a non-null element
    if (code == 0xff)
        return "NAK";
    if (data.size() < 1)
        return "Unknown error packet";
    return errorMessage(data.at(0));
}

} // namespace trk

