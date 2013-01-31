/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

// Todo item icons

// http://en.wikipedia.org/wiki/File:Information_icon_with_gradient_background.svg,
// public domain, tuned a bit
const char ICON_INFO[] = ":/todoplugin/images/info.png";

// Dummy, needs to be changed
const char ICON_TODO[] = ":/todoplugin/images/todo.png";

const char ICON_WARNING[] = ":/projectexplorer/images/compile_warning.png";
const char ICON_ERROR[] = ":/projectexplorer/images/compile_error.png";

// Public domain, I am the author
const char ICON_CURRENT_FILE[] = ":/todoplugin/images/current-file.png";
const char ICON_WHOLE_PROJECT[] = ":/todoplugin/images/whole-project.png";


// Settings entries
const char SETTINGS_GROUP[] = "TodoPlugin";
const char SCANNING_SCOPE[] = "ScanningScope";
const char ITEMS_DISPLAY_PLACE[] = "ItemsDisplayPlace";
const char KEYWORDS_LIST[] = "Keywords";

// TODO Output TreeWidget columns
enum OutputColumnIndex {
    OUTPUT_COLUMN_TEXT,
    OUTPUT_COLUMN_FILE,
    OUTPUT_COLUMN_LINE,
    OUTPUT_COLUMN_LAST
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
