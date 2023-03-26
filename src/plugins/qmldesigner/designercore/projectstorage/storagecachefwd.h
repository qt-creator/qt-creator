// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/smallstringfwd.h>

namespace QmlDesigner {

class NonLockingMutex;

template<typename Type,
         typename ViewType,
         typename IndexType,
         typename Storage,
         typename Mutex,
         bool (*compare)(ViewType, ViewType),
         typename CacheEntry>
class StorageCache;
} // namespace QmlDesigner
