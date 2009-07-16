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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "trkutils.h"

#include <QtCore/QDebug>

#define logMessage(s)  do { qDebug() << "TRKCLIENT: " << s; } while (0)

namespace trk {

QString TrkResult::toString() const
{
    QString res = stringFromByte(code) + "[" + stringFromByte(token);
    res.chop(1);
    return res + "] " + stringFromArray(data);
}

QByteArray frameMessage(byte command, byte token, const QByteArray &data)
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
    ushort encodedSize = encodedData.size() + 2; // 2 x 0x7e

    QByteArray ba;
    ba.append(char(0x01));
    ba.append(char(0x90));
    ba.append(char(encodedSize / 256));
    ba.append(char(encodedSize % 256));
    ba.append(char(0x7e));
    ba.append(encodedData);
    ba.append(char(0x7e));

    return ba;
}

TrkResult extractResult(QByteArray *buffer)
{
    TrkResult result;
    if (buffer->at(0) != 0x01 || byte(buffer->at(1)) != 0x90) {
        logMessage("*** ERROR READBUFFER INVALID (2): "
            << stringFromArray(*buffer)
            << int(buffer->at(0))
            << int(buffer->at(1))
            << buffer->size());
        return result;
    }
    ushort len = extractShort(buffer->data() + 2);

    //logMessage("   READ BUF: " << stringFromArray(*buffer));
    if (buffer->size() < len + 4) {
        logMessage("*** INCOMPLETE RESPONSE: "
            << stringFromArray(*buffer));
        return result;
    }

    if (byte(buffer->at(4)) != 0x7e) {
        logMessage("** ERROR READBUFFER BEGIN FRAME MARKER INVALID: "
            << stringFromArray(*buffer) << len);
        return result;
    }

    if (byte(buffer->at(4 + len - 1)) != 0x7e) {
        logMessage("** ERROR READBUFFER END FRAME MARKER INVALID: "
            << stringFromArray(*buffer) << len);
        return result;
    }

    // FIXME: what happens if the length contains 0xfe?
    // Assume for now that it passes unencoded!
    QByteArray data = decode7d(buffer->mid(5, len - 2));
    *buffer = buffer->mid(4 + len);

    byte sum = 0;
    for (int i = 0; i < data.size(); ++i) // 3 = 2 * 0xfe + sum
        sum += byte(data.at(i));
    if (sum != 0xff)
        logMessage("*** CHECKSUM ERROR: " << byte(sum));

    result.code = data.at(0);
    result.token = data.at(1);
    result.data = data.mid(2);
    //logMessage("   REST BUF: " << stringFromArray(*buffer));
    //logMessage("   CURR DATA: " << stringFromArray(data));
    //QByteArray prefix = "READ BUF:                                       ";
    //logMessage((prefix + "HEADER: " + stringFromArray(header).toLatin1()).data());
    return result;
}

ushort extractShort(const char *data)
{
    return data[0] * 256 + data[1];
}

uint extractInt(const char *data)
{
    uint res = data[0];
    res *= 256; res += data[1];
    res *= 256; res += data[2];
    res *= 256; res += data[3];
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

// FIXME: Use the QByteArray based version below?
QString stringFromByte(byte c)
{
    return QString("%1 ").arg(c, 2, 16, QChar('0'));
}

QString stringFromArray(const QByteArray &ba)
{
    QString str;
    QString ascii;
    for (int i = 0; i < ba.size(); ++i) {
        //if (i == 5 || i == ba.size() - 2)
        //    str += "  ";
        int c = byte(ba.at(i));
        str += QString("%1 ").arg(c, 2, 16, QChar('0'));
        if (i >= 8 && i < ba.size() - 2)
            ascii += QChar(c).isPrint() ? QChar(c) : QChar('.');
    }
    return str + "  " + ascii;
}


QByteArray formatByte(byte b)
{
    char buf[30];
    qsnprintf(buf, sizeof(buf) - 1, "%x ", b);
    return buf;
}

QByteArray formatShort(ushort s)
{
    char buf[30];
    qsnprintf(buf, sizeof(buf) - 1, "%x %x ", s / 256, s % 256);
    return buf;
}

QByteArray formatInt(uint i)
{
    char buf[30];
    int b3 = i % 256; i -= b3; i /= 256;
    int b2 = i % 256; i -= b2; i /= 256;
    int b1 = i % 256; i -= b1; i /= 256;
    int b0 = i % 256; i -= b0; i /= 256;
    qsnprintf(buf, sizeof(buf) - 1, "%x %x %x %x ", b0, b1, b2, b3);
    return buf;
}

QByteArray formatString(const QByteArray &str)
{
    QByteArray ba = formatShort(str.size());
    foreach (byte b, str)
        ba.append(formatByte(b));
    return ba;
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


} // namespace trk

