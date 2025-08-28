// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtconfigmacros.h>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace ClangFormat {
void setupCodeStyleFactory(QObject *guard);
} // namespace ClangFormat
