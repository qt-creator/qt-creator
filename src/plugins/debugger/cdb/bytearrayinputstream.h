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

#ifndef BYTEARRAYINPUTSTREAM_H
#define BYTEARRAYINPUTSTREAM_H

#include <QString>

namespace Debugger {
namespace Internal {

class ByteArrayInputStream
{
    Q_DISABLE_COPY(ByteArrayInputStream)

public:
    typedef void (ModifierFunc)(ByteArrayInputStream &s);

    explicit ByteArrayInputStream(QByteArray &ba);

    ByteArrayInputStream &operator<<(char a)              { m_target.append(a); return *this; }
    ByteArrayInputStream &operator<<(const QByteArray &a) { m_target.append(a); return *this; }
    ByteArrayInputStream &operator<<(const char *a)       { m_target.append(a); return *this; }
    ByteArrayInputStream &operator<<(const QString &a)    { m_target.append(a.toLatin1()); return *this; }

    ByteArrayInputStream &operator<<(int i) { appendInt(i); return *this; }
    ByteArrayInputStream &operator<<(unsigned i) { appendInt(i); return *this; }
    ByteArrayInputStream &operator<<(quint64 i) { appendInt(i); return *this; }
    ByteArrayInputStream &operator<<(qint64 i) { appendInt(i); return *this; }

    // Stream a modifier by invoking it
    ByteArrayInputStream &operator<<(ModifierFunc mf) { mf(*this); return *this; }

    void setHexPrefix(bool hp) { m_hexPrefix = hp; }
    bool hexPrefix() const     { return  m_hexPrefix; }
    void setIntegerBase(int b) { m_integerBase = b; }
    int integerBase() const    { return m_integerBase; }
    // Append a separator if required (target does not end with it)
    void appendSeparator(char c = ' ');

private:
    template <class IntType> void appendInt(IntType i);

    QByteArray &m_target;
    int m_integerBase;
    bool m_hexPrefix;
    int m_width;
};

template <class IntType>
void ByteArrayInputStream::appendInt(IntType i)
{
    const bool hexPrefix = m_integerBase == 16 && m_hexPrefix;
    if (hexPrefix)
        m_target.append("0x");
    const QByteArray n = QByteArray::number(i, m_integerBase);
    if (m_width > 0) {
        int pad = m_width - n.size();
        if (hexPrefix)
            pad -= 2;
        if (pad > 0)
            m_target.append(QByteArray(pad, '0'));
    }
    m_target.append(n);
}

// Streamable modifiers for ByteArrayInputStream
void hexPrefixOn(ByteArrayInputStream &bs);
void hexPrefixOff(ByteArrayInputStream &bs);
void hex(ByteArrayInputStream &bs);
void dec(ByteArrayInputStream &bs);
void blankSeparator(ByteArrayInputStream &bs);

// Bytearray parse helpers
QByteArray trimFront(QByteArray in);
QByteArray trimBack(QByteArray in);
QByteArray simplify(const QByteArray &inIn);

} // namespace Internal
} // namespace Debugger

#endif // BYTEARRAYINPUTSTREAM_H
