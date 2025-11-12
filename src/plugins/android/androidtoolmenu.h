// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace Android::Internal {

void setupAndroidToolsMenu();
Utils::FilePath manifestDir(TextEditor::TextEditorWidget *textEditorWidget);

} // namespace Android::Internal
