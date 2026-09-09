// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QObject)

namespace CMakeProjectManager::Internal {

void setupCMakeFormatter();

// Whether cmake-format, rather than the built-in formatter, is the one that
// lays a CMake file out.
bool cmakeFormatIsFormatter();

// Calls handler whenever that answer changes, for as long as guard lives.
void onFormatterChanged(QObject *guard, const std::function<void()> &handler);

} // CMakeProjectManager::Internal
