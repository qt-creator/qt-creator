// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <QColor>
#include <QHash>

namespace Core {

class CORE_EXPORT SearchResultColor
{
public:
    enum class Style { Default, Alt1, Alt2 };

    SearchResultColor() = default;
    SearchResultColor(const QColor &textBg, const QColor &textFg,
                      const QColor &highlightBg, const QColor &highlightFg,
                      const QColor &functionBg, const QColor &functionFg
                      )
        : textBackground(textBg), textForeground(textFg),
          highlightBackground(highlightBg), highlightForeground(highlightFg),
          containingFunctionBackground(functionBg),containingFunctionForeground(functionFg)
    {
        if (!highlightBackground.isValid())
            highlightBackground = textBackground;
        if (!highlightForeground.isValid())
            highlightForeground = textForeground;
        if (!containingFunctionBackground.isValid())
            containingFunctionBackground = textBackground;
        if (!containingFunctionForeground.isValid())
            containingFunctionForeground = textForeground;
    }

    friend auto qHash(SearchResultColor::Style style)
    {
        return QT_PREPEND_NAMESPACE(qHash(int(style)));
    }

    QColor textBackground;
    QColor textForeground;
    QColor highlightBackground;
    QColor highlightForeground;
    QColor containingFunctionBackground;
    QColor containingFunctionForeground;
};

using SearchResultColors = QHash<SearchResultColor::Style, SearchResultColor>;

} // namespace Core
