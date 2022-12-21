// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

namespace Core {
namespace Internal {

void CORE_EXPORT runSearch(QFutureInterface<LocatorFilterEntry> &future,
                              const QList<ILocatorFilter *> &filters,
                              const QString &searchText);

} // namespace Internal
} // namespace Core
