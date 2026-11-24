// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>

namespace Terminal {

class TerminalWidget;

namespace Internal {

void setupTerminalEditor(const std::function<void(TerminalWidget *)> &moveToPane);
bool openTerminalInEditor(TerminalWidget *terminal);
void closeAllTerminalEditors();

} // namespace Internal
} // namespace Terminal
