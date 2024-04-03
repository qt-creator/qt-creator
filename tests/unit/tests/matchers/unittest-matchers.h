// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>

#include <utils/smallstringio.h>

#include <QUrl>

QT_BEGIN_NAMESPACE

inline bool empty(const QString &string)
{
    return string.isEmpty();
}

inline bool empty(const QByteArray &bytetArray)
{
    return bytetArray.isEmpty();
}

inline bool empty(const QUrl &url)
{
    return url.isEmpty();
}

QT_END_NAMESPACE

namespace Internal {

template <typename StringType>
class EndsWithMatcher
{
 public:
    explicit EndsWithMatcher(const StringType& suffix) : m_suffix(suffix) {}

    template <typename CharType>
    bool MatchAndExplain(CharType *s, testing::MatchResultListener *listener) const
    {
        return s != NULL && MatchAndExplain(StringType(s), listener);
    }


    template <typename MatcheeStringType>
    bool MatchAndExplain(const MatcheeStringType& s,
                         testing::MatchResultListener* /* listener */) const
    {
        return s.endsWith(m_suffix);
    }

    void DescribeTo(::std::ostream* os) const
    {
        *os << "ends with " << m_suffix;
    }

    void DescribeNegationTo(::std::ostream* os) const
    {
        *os << "doesn't end with " << m_suffix;
    }

    EndsWithMatcher(const EndsWithMatcher &) = default;
    EndsWithMatcher &operator=(const EndsWithMatcher &) = delete;

private:
    const StringType m_suffix;
};

class QStringEndsWithMatcher
{
public:
    explicit QStringEndsWithMatcher(const QString &suffix)
        : m_suffix(suffix)
    {}

    template<typename MatcheeStringType>
    bool MatchAndExplain(const MatcheeStringType &s, testing::MatchResultListener * /* listener */) const
    {
        return s.endsWith(m_suffix);
    }

    void DescribeTo(::std::ostream *os) const
    {
        *os << "ends with " << testing::PrintToString(m_suffix);
    }

    void DescribeNegationTo(::std::ostream *os) const
    {
        *os << "doesn't end with " << testing::PrintToString(m_suffix);
    }

private:
    const QString m_suffix;
};

template<typename StringType>
class StartsWithMatcher
{
public:
    explicit StartsWithMatcher(const StringType &prefix)
        : m_prefix(prefix)
    {}

    template<typename CharType>
    bool MatchAndExplain(CharType *s, testing::MatchResultListener *listener) const
    {
        return s != NULL && MatchAndExplain(StringType(s), listener);
    }

    template<typename MatcheeStringType>
    bool MatchAndExplain(const MatcheeStringType &s, testing::MatchResultListener * /* listener */) const
    {
        return s.startsWith(m_prefix);
    }

    void DescribeTo(::std::ostream *os) const { *os << "ends with " << m_prefix; }

    void DescribeNegationTo(::std::ostream *os) const { *os << "doesn't end with " << m_prefix; }

    StartsWithMatcher(const StartsWithMatcher &) = default;
    StartsWithMatcher &operator=(const StartsWithMatcher &) = delete;

private:
    const StringType m_prefix;
};

class QStringStartsWithMatcher
{
public:
    explicit QStringStartsWithMatcher(const QString &prefix)
        : m_prefix(prefix)
    {}

    template<typename MatcheeStringType>
    bool MatchAndExplain(const MatcheeStringType &s, testing::MatchResultListener * /* listener */) const
    {
        return s.startsWith(m_prefix);
    }

    void DescribeTo(::std::ostream *os) const
    {
        *os << "ends with " << testing::PrintToString(m_prefix);
    }

    void DescribeNegationTo(::std::ostream *os) const
    {
        *os << "doesn't end with " << testing::PrintToString(m_prefix);
    }

private:
    const QString m_prefix;
};

class IsEmptyMatcher
{
public:
    template<typename Type>
    bool MatchAndExplain(const Type &value, testing::MatchResultListener *) const
    {
        using std::empty;

        return empty(value);
    }

    void DescribeTo(std::ostream *os) const { *os << "is empty"; }

    void DescribeNegationTo(std::ostream *os) const { *os << "isn't empty"; }
};

class IsNullMatcher
{
public:
    template<typename Type>
    bool MatchAndExplain(const Type &value, testing::MatchResultListener *) const
    {
        if constexpr (std::is_pointer_v<Type>)
            return value == nullptr;
        else
            return value.isNull();
    }

    void DescribeTo(std::ostream *os) const { *os << "is null"; }

    void DescribeNegationTo(std::ostream *os) const { *os << "isn't null"; }
};

class IsValidMatcher
{
public:
    template<typename Type>
    bool MatchAndExplain(const Type &value, testing::MatchResultListener *) const
    {
        if constexpr (std::is_pointer_v<Type>)
            return value != nullptr;
        else
            return value.isValid();
    }

    void DescribeTo(std::ostream *os) const { *os << "is null"; }

    void DescribeNegationTo(std::ostream *os) const { *os << "isn't null"; }
};

} // namespace Internal

inline auto EndsWith(const Utils::SmallStringView &suffix)
{
    return ::testing::PolymorphicMatcher(Internal::EndsWithMatcher(suffix));
}

inline auto EndsWith(const QStringView &suffix)
{
    return ::testing::PolymorphicMatcher(Internal::QStringEndsWithMatcher(suffix.toString()));
}

inline auto StartsWith(const Utils::SmallStringView &prefix)
{
    return ::testing::PolymorphicMatcher(Internal::StartsWithMatcher(prefix));
}

inline auto StartsWith(const QStringView &prefix)
{
    return ::testing::PolymorphicMatcher(Internal::QStringStartsWithMatcher(prefix.toString()));
}

inline auto IsEmpty()
{
    return ::testing::PolymorphicMatcher(Internal::IsEmptyMatcher());
}

inline auto IsNull()
{
    return ::testing::PolymorphicMatcher(Internal::IsNullMatcher());
}

inline auto IsValid()
{
    return ::testing::PolymorphicMatcher(Internal::IsValidMatcher());
}
