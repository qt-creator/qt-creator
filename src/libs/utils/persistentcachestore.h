// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "expected.h"
#include "store.h"
#include "storekey.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT PersistentCacheStore
{
public:
    static expected_str<Store> byKey(const Key &cacheKey);
    static expected_str<void> write(const Key &cacheKey, const Store &store);
    static expected_str<void> clear(const Key &cacheKey);
};

} // namespace Utils
