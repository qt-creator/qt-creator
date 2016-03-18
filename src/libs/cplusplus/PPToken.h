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

#pragma once

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
