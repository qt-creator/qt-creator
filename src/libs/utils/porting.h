/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "utils_global.h"

#ifdef QT_GUI_LIB
#include <QEnterEvent>
#endif
#include <QString>

namespace Utils {

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
constexpr QString::SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
#else
constexpr Qt::SplitBehaviorFlags SkipEmptyParts = Qt::SkipEmptyParts;
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using QHashValueType = uint;
#else
using QHashValueType = size_t;
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using QtSizeType = int;
#else
using QtSizeType = qsizetype;
#endif

// StringView - either QStringRef or QStringView
// Can be used where it is not possible to completely switch to QStringView
// For example where QString::splitRef / QStringView::split is essential.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using StringView = QStringRef;
#else
using StringView = QStringView;
#endif

inline StringView make_stringview(const QString &s)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return QStringRef(&s);
#else
    return QStringView(s);
#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
// QStringView::mid in Qt5 does not do bounds checking, in Qt6 it does
inline QStringView midView(const QString &s, int offset, int length)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const int size = s.size();
    if (offset > size)
        return {};
    if (offset < 0) {
        if (length < 0 || length + offset >= size)
            return QStringView(s);
        if (length + offset <= 0)
            return {};
        return QStringView(s).left(length + offset);
    } else if (length > size - offset)
        return QStringView(s).mid(offset);
    return QStringView(s).mid(offset, length);
#else
    return QStringView(s).mid(offset, length);
#endif
}
#endif

#ifdef QT_GUI_LIB
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using EnterEvent = QEvent;
#else
using EnterEvent = QEnterEvent;
#endif
#endif

} // namespace Utils
