// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

void clearHeaderSourceCache();

#ifdef WITH_TESTS
QObject *createCppHeaderSourceTest();
#endif

} // CppEditor::Internal
