#include "PPToken.h"

#include <cstring>

using namespace CPlusPlus;

bool ByteArrayRef::startsWith(const char *s) const
{
    int l = std::strlen(s);
    if (l > m_length)
        return false;
    return !qstrncmp(start(), s, l);
}

int ByteArrayRef::count(char ch) const
{
    int num = 0;
    const char *b = start();
    const char *i = b + m_length;
    while (i != b)
        if (*--i == ch)
            ++num;
    return num;
}

void Internal::PPToken::squeezeSource()
{
    if (hasSource()) {
        m_src = m_src.mid(offset, f.length);
        m_src.squeeze();
        offset = 0;
    }
}
