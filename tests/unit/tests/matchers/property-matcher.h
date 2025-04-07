// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>

#include <concepts>
#include <type_traits>

namespace internal {

template<typename Property, typename Matcher, typename... Arguments>
class PropertyMatcher
{
public:
    PropertyMatcher(Property property, const Matcher &matcher, Arguments... arguments)
        : m_property(property)
        , m_matcher(matcher)
        , m_whose_property()
        , m_arguments{arguments...}
    {}

    PropertyMatcher(const std::string &whose_property,
                    Property property,
                    const Matcher &matcher,
                    Arguments... arguments)
        : m_property(property)
        , m_matcher(matcher)
        , m_whose_property(whose_property)
        , m_arguments{arguments...}
    {}

    void DescribeTo(::std::ostream *os) const
    {
        *os << "is an object " << m_whose_property;
        m_matcher.DescribeTo(os);
    }

    void DescribeNegationTo(::std::ostream *os) const
    {
        *os << "is an object " << m_whose_property;
        m_matcher.DescribeNegationTo(os);
    }

    template<typename T>
    bool MatchAndExplain(const T &value, testing::MatchResultListener *listener) const
    {
        *listener << "which points to an object ";

        auto result = std::apply(
            [&](auto &&...arguments) { return std::invoke(m_property, value, arguments...); },
            m_arguments);
        return m_matcher.MatchAndExplain(result, listener->stream());
    }

private:
    Property m_property;
    Matcher m_matcher;
    std::string m_whose_property;
    std::tuple<Arguments...> m_arguments;
};

template<typename PropertyMatcher>
concept matcher = std::derived_from<PropertyMatcher, testing::MatcherDescriberInterface>
                  || requires { typename PropertyMatcher::is_gtest_matcher; };

} // namespace internal

template<typename PropertyType, internal::matcher PropertyMatcher, typename... Arguments>
inline auto Property(const std::string &propertyName,
                     PropertyType property,
                     const PropertyMatcher &matchingType,
                     Arguments... arguments)
//    requires(sizeof...(Arguments) >= 1)
{
    using namespace std::string_literals;

    return testing::MakePolymorphicMatcher(
        internal::PropertyMatcher<PropertyType, PropertyMatcher, Arguments...>(
            "whose property `"s + propertyName + "` "s,
            property,
            matchingType,
            std::forward<Arguments>(arguments)...));
}
