/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
