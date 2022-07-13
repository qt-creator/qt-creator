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

#include <QString>

namespace Utils {

// FIXME: Remove when downstream has been adapted

using QHashValueType = size_t;
using QtSizeType = qsizetype;
using StringView = QStringView;

inline QStringView make_stringview(const QString &s)
{
    return QStringView(s);
}

// QStringView::mid in Qt5 does not do bounds checking, in Qt6 it does
inline QStringView midView(const QString &s, int offset, int length = -1)
{
    return QStringView(s).mid(offset, length);
}

} // namespace Utils
