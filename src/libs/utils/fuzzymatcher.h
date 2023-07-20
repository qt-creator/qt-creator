// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 BlackBerry Limited <qt@blackberry.com>
// Copyright (C) 2017 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QList>

QT_BEGIN_NAMESPACE
class QRegularExpression;
class QRegularExpressionMatch;
class QString;
QT_END_NAMESPACE

class QTCREATOR_UTILS_EXPORT FuzzyMatcher
{
public:
    enum class CaseSensitivity {
        CaseInsensitive,
        CaseSensitive,
        FirstLetterCaseSensitive
    };

    class HighlightingPositions {
    public:
        QList<int> starts;
        QList<int> lengths;
    };

    static QRegularExpression createRegExp(
        const QString &pattern,
        CaseSensitivity caseSensitivity = CaseSensitivity::CaseInsensitive,
        bool multiWord = false);
    static QRegularExpression createRegExp(const QString &pattern,
                                           Qt::CaseSensitivity caseSensitivity,
                                           bool multiWord);
    static HighlightingPositions highlightingPositions(const QRegularExpressionMatch &match);
};
