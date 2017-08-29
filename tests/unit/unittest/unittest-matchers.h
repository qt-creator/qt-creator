/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <gmock/gmock-matchers.h>

#include <utils/smallstringio.h>

namespace UnitTests {

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

    EndsWithMatcher(EndsWithMatcher const &) = default;
    EndsWithMatcher &operator=(EndsWithMatcher const &) = delete;

private:
    const StringType m_suffix;
};

inline
testing::PolymorphicMatcher<EndsWithMatcher<Utils::SmallString> >
EndsWith(const Utils::SmallString &suffix)
{
  return testing::MakePolymorphicMatcher(EndsWithMatcher<Utils::SmallString>(suffix));
}
}
