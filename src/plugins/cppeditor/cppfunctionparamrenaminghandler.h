// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace CppEditor {
class CppEditorWidget;

namespace Internal {
class CppLocalRenaming;

// Watches local renamings and adapts the declaration if a function parameter was renamed.
class CppFunctionParamRenamingHandler
{
public:
    CppFunctionParamRenamingHandler(CppEditorWidget &editorWidget, CppLocalRenaming &localRenaming);
    ~CppFunctionParamRenamingHandler();

private:
    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace CppEditor
