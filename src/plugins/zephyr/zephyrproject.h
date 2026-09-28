// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace Zephyr::Internal {

void setupZephyrProject();

#ifdef WITH_TESTS
QObject *createZephyrCompileDatabaseTest();
#endif

} // namespace Zephyr::Internal
