#ifndef CPLUSPLUS_INTERNAL_PPTOKEN_H
#define CPLUSPLUS_INTERNAL_PPTOKEN_H

#include <CPlusPlus.h>
#include <Token.h>

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
    {
        Q_ASSERT(ref);
        Q_ASSERT(offset >= 0);
        Q_ASSERT(length >= 0);
        Q_ASSERT(offset + length <= ref->size());
    }

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
    { return qstrncmp(m_start, other, qstrlen(other)) == 0; }
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
    { return bufferStart() + offset; }

    ByteArrayRef asByteArrayRef() const
    { return ByteArrayRef(&m_src, offset, length()); }

private:
    QByteArray m_src;
};

} // namespace Internal
} // namespace CPlusPlus

#endif // CPLUSPLUS_INTERNAL_PPTOKEN_H
