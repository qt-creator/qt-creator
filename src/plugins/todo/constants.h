// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Todo::Constants {

inline constexpr char TODO_SETTINGS[] = "TodoSettings";

// Settings entries
inline constexpr char SETTINGS_GROUP[] = "TodoPlugin";
inline constexpr char SCANNING_SCOPE[] = "ScanningScope";
inline constexpr char ITEMS_DISPLAY_PLACE[] = "ItemsDisplayPlace";
inline constexpr char KEYWORDS_LIST[] = "Keywords";
inline constexpr char OUTPUT_PANE_TEXT_WIDTH[] = "OutputPaneTextColumnWidth";
inline constexpr char OUTPUT_PANE_FILE_WIDTH[] = "OutputPaneFileColumnWidth";
inline constexpr char SETTINGS_NAME_KEY[] = "TodoProjectSettings";
inline constexpr char EXCLUDES_LIST_KEY[] = "ExcludesList";
inline constexpr char USE_GLOBAL_KEY[]    = "UseGlobal";

// TODO Output TreeWidget columns
enum OutputColumnIndex {
    OUTPUT_COLUMN_TEXT,
    OUTPUT_COLUMN_FILE,
    OUTPUT_COLUMN_LINE,
    OUTPUT_COLUMN_COUNT
};

inline constexpr int OUTPUT_TOOLBAR_SPACER_WIDTH = 25;

inline constexpr int OUTPUT_PANE_UPDATE_INTERVAL = 2000;

inline constexpr char FILTER_KEYWORD_NAME[] = "filterKeywordName";

} // namespace Todo::Constants
