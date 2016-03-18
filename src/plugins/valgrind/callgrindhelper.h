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

#include <QLocale>

QT_BEGIN_NAMESPACE
class QColor;
class QString;
QT_END_NAMESPACE

namespace Valgrind {
namespace Internal {

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

} // namespace Internal
} // namespace Valgrind
