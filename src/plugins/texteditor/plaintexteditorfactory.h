// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace TextEditor {

TEXTEDITOR_EXPORT BaseTextEditor *createPlainTextEditor();

namespace Internal { void setupPlainTextEditor(); }

} // TextEditor
