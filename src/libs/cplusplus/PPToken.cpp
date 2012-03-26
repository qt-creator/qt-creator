#include "PPToken.h"

#include <cstring>

using namespace CPlusPlus::Internal;

ByteArrayRef::ByteArrayRef()
    : m_ref(0)
    , m_offset(0)
    , m_length(0)
{}

bool ByteArrayRef::startsWith(const char *s) const
{
    int l = std::strlen(s);
    if (l > m_length)
        return false;
    return !qstrncmp(start(), s, l);
}

int ByteArrayRef::count(char ch) const
{
    if (!m_ref)
        return 0;

    int num = 0;
    const char *b = start();
    const char *i = b + m_length;
    while (i != b)
        if (*--i == ch)
            ++num;
    return num;
}

PPToken::PPToken()
{}

void PPToken::squeeze()
{
    if (isValid()) {
        m_src = m_src.mid(offset, length());
        m_src.squeeze();
        offset = 0;
    }
}
