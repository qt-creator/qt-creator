// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#ifdef WITH_TESTS

#include <QObject>

namespace TextEditor::Internal {

QObject *createCodeAssistTests();

} // TextEditor::Internal

#endif // WITH_TESTS
