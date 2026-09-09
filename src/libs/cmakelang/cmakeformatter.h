// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeedit.h"
#include "cmakelang.h"
#include "cmakestyle.h"

#include <QList>

namespace CMakeLang {

// The edits that give a file the whitespace the style asks for: every line at
// the level the Indentation names, one space between the arguments of a call,
// no whitespace at the end of a line and one newline at the end of the file.
//
// Only what stands between two tokens ever changes, so the comments, the
// strings and the bracket arguments of the file come out of it untouched, and
// the line breaks its author made stay where they are: a call whose arguments
// are one to a line keeps them that way, however short they are.
CMAKELANG_EXPORT QList<Edit> formattingEdits(QStringView source, const Style &style);

} // namespace CMakeLang
