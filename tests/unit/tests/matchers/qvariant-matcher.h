// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVariant>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>

template<typename Type>
class IsQVariantTypeMatcher
{
public:
    using is_gtest_matcher = void;

    bool MatchAndExplain(const QVariant &value, std::ostream *) const
    {
        return QMetaType::fromType<Type>() == value.metaType();
    }

    void DescribeTo(std::ostream *os) const { *os << "is qvariant of type"; }

    void DescribeNegationTo(std::ostream *os) const { *os << "is not qvariant of type"; }
};

template<typename Type>
testing::Matcher<QVariant> IsQVariantType()
{
    return IsQVariantTypeMatcher<Type>();
}

template<typename Type, typename Matcher>
auto IsQVariant(const Matcher &matcher)
{
    return AllOf(IsQVariantType<Type>(), Property(&QVariant::value<Type>, matcher));
}

template<typename Matcher>
auto QVariantIsValid(const Matcher &matcher)
{
    return Property(&QVariant::isValid, matcher);
}
