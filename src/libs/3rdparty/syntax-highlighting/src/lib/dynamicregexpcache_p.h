/*
    SPDX-FileCopyrightText: 2023 Jonathan Poelen <jonathan.poelen+kde@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_DYNAMICREGEXPCACHE_P_H
#define KSYNTAXHIGHLIGHTING_DYNAMICREGEXPCACHE_P_H

#include <QCache>
#include <QRegularExpression>
#include <QString>

#include <utility>

namespace KSyntaxHighlighting
{

class DynamicRegexpCache
{
public:
    const QRegularExpression &compileRegexp(QString &&pattern, QRegularExpression::PatternOptions patternOptions)
    {
        const auto key = std::pair{std::move(pattern), patternOptions};
        if (const auto regexp = m_cache.object(key)) {
            return *regexp;
        }
        auto regexp = new QRegularExpression(key.first, patternOptions);
        m_cache.insert(key, regexp);
        return *regexp;
    }

private:
    QCache<std::pair<QString, QRegularExpression::PatternOptions>, QRegularExpression> m_cache;
};

}

#endif
