// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "store.h"

#include "algorithm.h"

namespace Utils {

KeyList keysFromStrings(const QStringList &list)
{
    return transform(list, &keyFromString);
}

QStringList stringsFromKeys(const KeyList &list)
{
    return transform(list, &stringFromKey);
}

} // Utils
