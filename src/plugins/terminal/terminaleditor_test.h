// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Terminal {

class TerminalPane;

namespace Internal {

QObject *createTerminalEditorTest(TerminalPane *pane);

} // namespace Internal
} // namespace Terminal
