// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Todo {
namespace Constants {

const char TODO_SETTINGS[] = "TodoSettings";

// Settings entries
const char SETTINGS_GROUP[] = "TodoPlugin";
const char SCANNING_SCOPE[] = "ScanningScope";
const char ITEMS_DISPLAY_PLACE[] = "ItemsDisplayPlace";
const char KEYWORDS_LIST[] = "Keywords";
const char OUTPUT_PANE_TEXT_WIDTH[] = "OutputPaneTextColumnWidth";
const char OUTPUT_PANE_FILE_WIDTH[] = "OutputPaneFileColumnWidth";
const char SETTINGS_NAME_KEY[] = "TodoProjectSettings";
const char EXCLUDES_LIST_KEY[] = "ExcludesList";

// TODO Output TreeWidget columns
enum OutputColumnIndex {
    OUTPUT_COLUMN_TEXT,
    OUTPUT_COLUMN_FILE,
    OUTPUT_COLUMN_LINE,
    OUTPUT_COLUMN_COUNT
};

const int OUTPUT_TOOLBAR_SPACER_WIDTH = 25;

const int OUTPUT_PANE_UPDATE_INTERVAL = 2000;

const char FILTER_KEYWORD_NAME[] = "filterKeywordName";

} // namespace Constants
} // namespace Todo
