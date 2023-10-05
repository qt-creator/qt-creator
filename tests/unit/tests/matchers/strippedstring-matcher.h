// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "version-matcher.h"

#include <projectstorage/projectstoragetypes.h>

#include <QRegularExpression>

#include <algorithm>

namespace Internal {
class StippedStringEqMatcher
{
public:
    explicit StippedStringEqMatcher(QString content)
        : m_content(strip(std::move(content)))
    {}

    bool MatchAndExplain(const QString &s, testing::MatchResultListener *listener) const
    {
        auto strippedContent = strip(s);
        bool isEqual = m_content == strippedContent;

        if (!isEqual) {
            auto [found1, found2] = std::mismatch(strippedContent.begin(),
                                                  strippedContent.end(),
                                                  m_content.begin(),
                                                  m_content.end());
            if (found1 != strippedContent.end() && found2 != m_content.end()) {
                *listener << "\ncurrent value mismatch start: \n"
                          << QStringView{found1, strippedContent.end()};
                *listener << "\nexpected value mismatch start: \n"
                          << QStringView{found2, m_content.end()};
            }
        }

        return isEqual;
    }

    static QString strip(QString s)
    {
        s.replace('\n', ' ');

        auto newEnd = std::unique(s.begin(), s.end(), [](QChar first, QChar second) {
            return first.isSpace() && second.isSpace();
        });

        s.erase(newEnd, s.end());

        static QRegularExpression regex1{R"xx((\W)\s)xx"};
        s.replace(regex1, R"xx(\1)xx");

        static QRegularExpression regex2{R"xx(\s(\W))xx"};
        s.replace(regex2, R"xx(\1)xx");

        return s.trimmed();
    }

    void DescribeTo(::std::ostream *os) const
    {
        *os << "has stripped content " << testing::PrintToString(m_content);
    }

    void DescribeNegationTo(::std::ostream *os) const
    {
        *os << "doesn't have stripped content  " << testing::PrintToString(m_content);
    }

private:
    const QString m_content;
};

} // namespace Internal

inline auto StrippedStringEq(const QStringView &content)
{
    return ::testing::PolymorphicMatcher(Internal::StippedStringEqMatcher(content.toString()));
}
