// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "store.h"

#include "algorithm.h"

namespace Utils {

KeyList keyListFromStringList(const QStringList &list)
{
#ifdef QTC_USE_STORE
    return transform(list, [](const QString &str) { return str.toUtf8(); });
#else
    return list;
#endif
}

} // Utils
