// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    PPToken(QByteArray &&src)
        : m_src(std::move(src))
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

    unsigned originalOffset() const
    { return m_originalOffset != -1 ? m_originalOffset : byteOffset; }

private:
    QByteArray m_src;

    // TODO: We may or may not be able to get rid of this member. In order to find out,
    //       all code calling this class' accessors (including the parent class'
    //       bytes* and utf16* functions) has to be looked at. Essentially, it boils
    //       down to whether there are contexts where an object of this class is used
    //       and the original "global" string is no longer available. (If not, then the
    //       m_src member would also not be needed.)
    int m_originalOffset = -1;
};

} // namespace Internal
} // namespace CPlusPlus
