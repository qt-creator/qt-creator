/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QtGlobal>

namespace Todo {
namespace Constants {

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
