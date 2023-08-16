// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLocale>

QT_BEGIN_NAMESPACE
class QColor;
class QString;
QT_END_NAMESPACE

namespace Valgrind::Internal {

namespace CallgrindHelper
{
    /**
     * Returns color for a specific string, the string<->color mapping is cached
     */
    QColor colorForString(const QString &text);

    /**
     * Returns color for a specific cost ratio
     * \param ratio The cost ratio, ratio should be of [0,1]
     */
    QColor colorForCostRatio(qreal ratio);

    /**
     * Returns a proper percent representation of a float limited to 5 chars
     */
    QString toPercent(float costs, const QLocale &locale = QLocale());
}

} // namespace Valgrind::Internal
