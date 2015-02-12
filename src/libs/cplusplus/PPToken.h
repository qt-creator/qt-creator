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

#ifndef CPLUSPLUS_INTERNAL_PPTOKEN_H
#define CPLUSPLUS_INTERNAL_PPTOKEN_H

#include <cplusplus/CPlusPlus.h>
#include <cplusplus/Token.h>

#include <QByteArray>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ByteArrayRef
{
public:
    ByteArrayRef()
        : m_start("")
        , m_length(0)
    {}

    ByteArrayRef(const QByteArray *ref)
        : m_start(ref->constData())
        , m_length(ref->length())
    {}

    ByteArrayRef(const char *start, int length)
        : m_start(start)
        , m_length(length)
    {}

    ByteArrayRef(const QByteArray *ref, int offset, int length)
        : m_start(ref->constData() + offset)
        , m_length(length)
    {}

    inline const char *start() const
    { return m_start; }

    inline int length() const
    { return m_length; }

    inline int size() const
    { return length(); }

    inline char at(int pos) const
    { return pos >= 0 && pos < m_length ? m_start[pos] : '\0'; }

    inline char operator[](int pos) const
    { return at(pos); }

    QByteArray toByteArray() const
    { return QByteArray(m_start, m_length); }

    bool operator==(const QByteArray &other) const
    { return m_length == other.length() && !qstrncmp(m_start, other.constData(), m_length); }
    bool operator!=(const QByteArray &other) const
    { return !this->operator==(other); }

    bool operator==(const char *other) const
    { return m_length == (int) qstrlen(other) && !qstrncmp(m_start, other, m_length); }
    bool operator!=(const char *other) const
    { return !this->operator==(other); }

    bool startsWith(const char *ch) const;

    int count(char c) const;

private:
    const char *m_start;
    const int m_length;
};

inline bool operator==(const QByteArray &other, const ByteArrayRef &ref)
{ return ref == other; }

inline bool operator!=(const QByteArray &other, const ByteArrayRef &ref)
{ return ref != other; }

namespace Internal {

class CPLUSPLUS_EXPORT PPToken: public Token
{
public:
    PPToken() {}

    PPToken(const QByteArray &src)
        : m_src(src)
    {}

    void setSource(const QByteArray &src)
    { m_src = src; }

    const QByteArray &source() const
    { return m_src; }

    bool hasSource() const
    { return !m_src.isEmpty(); }

    void squeezeSource();

    const char *bufferStart() const
    { return m_src.constData(); }

    const char *tokenStart() const
    { return bufferStart() + byteOffset; }

    ByteArrayRef asByteArrayRef() const
    { return ByteArrayRef(&m_src, byteOffset, bytes()); }

private:
    QByteArray m_src;
};

} // namespace Internal
} // namespace CPlusPlus

#endif // CPLUSPLUS_INTERNAL_PPTOKEN_H
