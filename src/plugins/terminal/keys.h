// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <vterm_keycodes.h>

#include <QKeyEvent>

namespace Terminal::Internal {

VTermKey qtKeyToVTerm(Qt::Key key, bool keypad);
VTermModifier qtModifierToVTerm(Qt::KeyboardModifiers mod);

} // namespace Terminal::Internal
