// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace DiffEditor::Constants {

inline constexpr char DIFF_EDITOR_PLUGIN[] = "DiffEditorPlugin";

inline constexpr char DIFF_EDITOR_ID[] = "Diff Editor";
inline constexpr char DIFF_EDITOR_MIMETYPE[] = "text/x-patch";
inline constexpr char C_DIFF_EDITOR_DESCRIPTION[] = "DiffEditor.Description";
inline constexpr char SIDE_BY_SIDE_VIEW_ID[] = "DiffEditor.SideBySide";
inline constexpr char UNIFIED_VIEW_ID[] = "DiffEditor.Unified";
inline constexpr char SELECT_ENCODING[] = "DiffEditor.SelectEncoding";

inline constexpr char G_TOOLS_DIFF[] = "QtCreator.Group.Tools.Diff";

// The inline diff's toolbar state, persisted globally. Here rather than in
// inlinediff.cpp because the autotests pin them to a known value.
inline constexpr char INLINE_DIFF_VIEW_MODE_KEY[] = "DiffEditor/InlineDiffViewMode";
inline constexpr char INLINE_DIFF_COLLAPSE_KEY[] = "DiffEditor/InlineDiffCollapseUnchanged";
inline constexpr char INLINE_DIFF_IGNORE_WHITESPACE_KEY[]
    = "DiffEditor/InlineDiffIgnoreWhitespace";

} // namespace DiffEditor::Constants
