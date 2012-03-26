#ifndef CPLUSPLUS_INTERNAL_PPTOKEN_H
#define CPLUSPLUS_INTERNAL_PPTOKEN_H

#include <CPlusPlus.h>
#include <Token.h>

#include <QByteArray>

namespace CPlusPlus {
namespace Internal {

class CPLUSPLUS_EXPORT ByteArrayRef
{
public:
    ByteArrayRef();

    ByteArrayRef(const QByteArray *ref)
        : m_ref(ref)
        , m_offset(0)
        , m_length(ref->length())
    {}

    ByteArrayRef(const QByteArray *ref, int offset, int length)
        : m_ref(ref)
        , m_offset(offset)
        , m_length(length)
    {
        Q_ASSERT(ref);
        Q_ASSERT(offset >= 0);
        Q_ASSERT(length >= 0);
        Q_ASSERT(offset + length <= ref->size());
    }

    inline const char *start() const
    { return m_ref ? m_ref->constData() + m_offset : 0; }

    inline int length() const
    { return m_length; }

    inline int size() const
    { return length(); }

    inline char at(int pos) const
    { return m_ref && pos >= 0 && pos < m_length ? m_ref->at(m_offset + pos) : '\0'; }

    inline char operator[](int pos) const
    { return at(pos); }

    QByteArray toByteArray() const
    { return m_ref ? QByteArray(m_ref->constData() + m_offset, m_length) : QByteArray(); }

    bool operator==(const QByteArray &other) const
    { return m_ref ? (m_length == other.length() && !qstrncmp(m_ref->constData() + m_offset, other.constData(), m_length)) : false; }
    bool operator!=(const QByteArray &other) const
    { return !this->operator==(other); }

    bool startsWith(const char *ch) const;

    int count(char c) const;

private:
    const QByteArray *m_ref;
    int m_offset;
    int m_length;
};

inline bool operator==(const QByteArray &other, const ByteArrayRef &ref)
{ return ref == other; }

inline bool operator!=(const QByteArray &other, const ByteArrayRef &ref)
{ return ref != other; }

class CPLUSPLUS_EXPORT PPToken: public Token
{
public:
    PPToken();

    PPToken(const QByteArray &src)
        : m_src(src)
    {}

    void setSource(const QByteArray &src)
    { m_src = src; }

    const QByteArray &source() const
    { return m_src; }

    const char *start() const
    { return m_src.constData() + offset; }

    ByteArrayRef asByteArrayRef() const
    { return ByteArrayRef(&m_src, offset, length()); }

    bool isValid() const
    { return !m_src.isEmpty(); }

    void squeeze();

private:
    QByteArray m_src;
};

} // namespace Internal
} // namespace CPlusPlus

#endif // CPLUSPLUS_INTERNAL_PPTOKEN_H
