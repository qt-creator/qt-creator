// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Android::Internal {

void registerAndroidMcpTools();

#ifdef WITH_TESTS
QObject *createAndroidMcpSupportTest();
#endif

} // namespace Android::Internal
