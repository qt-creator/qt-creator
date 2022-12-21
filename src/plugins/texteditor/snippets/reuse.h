// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QLatin1String>

namespace TextEditor {
namespace Internal {

const QLatin1String kTrue("true");
const QLatin1String kFalse("false");

inline bool toBool(const QString &s)
{
    if (s == kTrue)
        return true;
    return false;
}

inline QString fromBool(bool b)
{
    if (b)
        return kTrue;
    return kFalse;
}

} // Internal
} // TextEditor
