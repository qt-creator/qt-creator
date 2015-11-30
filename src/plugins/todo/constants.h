/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QtGlobal>

namespace Todo {
namespace Constants {

// Default todo item background colors
const char COLOR_TODO_BG[] = "#ccffcc";
const char COLOR_WARNING_BG[] = "#ffffcc";
const char COLOR_FIXME_BG[] = "#ffcccc";
const char COLOR_BUG_BG[] = "#ffcccc";
const char COLOR_NOTE_BG[] = "#e0ebff";

// Dummy, needs to be changed
const char ICON_TODO[] = ":/todoplugin/images/todo.png";

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

const char OUTPUT_COLUMN_TEXT_TITLE[] = QT_TRANSLATE_NOOP("Todo::Internal::TodoItemsModel", "Description");
const char OUTPUT_COLUMN_FILE_TITLE[] = QT_TRANSLATE_NOOP("Todo::Internal::TodoItemsModel", "File");
const char OUTPUT_COLUMN_LINE_TITLE[] = QT_TRANSLATE_NOOP("Todo::Internal::TodoItemsModel", "Line");

const int OUTPUT_TOOLBAR_SPACER_WIDTH = 25;

const int OUTPUT_PANE_UPDATE_INTERVAL = 2000;
const char OUTPUT_PANE_TITLE[] = QT_TRANSLATE_NOOP("Todo::Internal::TodoOutputPane", "To-Do Entries");

} // namespace Constants
} // namespace Todo

#endif // CONSTANTS_H
