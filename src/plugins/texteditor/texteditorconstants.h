/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef TEXTEDITORCONSTANTS_H
#define TEXTEDITORCONSTANTS_H

#include <QtCore/QtGlobal>

namespace TextEditor {
namespace Constants {

const char * const C_TEXTEDITOR          = "Text Editor";
const char * const COMPLETE_THIS         = "TextEditor.CompleteThis";
const char * const QUICKFIX_THIS         = "TextEditor.QuickFix";
const char * const VISUALIZE_WHITESPACE  = "TextEditor.VisualizeWhitespace";
const char * const CLEAN_WHITESPACE      = "TextEditor.CleanWhitespace";
const char * const TEXT_WRAPPING         = "TextEditor.TextWrapping";
const char * const UN_COMMENT_SELECTION  = "TextEditor.UnCommentSelection";
const char * const REFORMAT              = "TextEditor.Reformat";
const char * const COLLAPSE              = "TextEditor.Collapse";
const char * const EXPAND                = "TextEditor.Expand";
const char * const UN_COLLAPSE_ALL       = "TextEditor.UnCollapseAll";
const char * const AUTO_INDENT_SELECTION = "TextEditor.AutoIndentSelection";
const char * const INCREASE_FONT_SIZE    = "TextEditor.IncreaseFontSize";
const char * const DECREASE_FONT_SIZE    = "TextEditor.DecreaseFontSize";
const char * const RESET_FONT_SIZE    = "TextEditor.ResetFontSize";
const char * const GOTO_BLOCK_START      = "TextEditor.GotoBlockStart";
const char * const GOTO_BLOCK_START_WITH_SELECTION = "TextEditor.GotoBlockStartWithSelection";
const char * const GOTO_BLOCK_END        = "TextEditor.GotoBlockEnd";
const char * const GOTO_BLOCK_END_WITH_SELECTION = "TextEditor.GotoBlockEndWithSelection";
const char * const SELECT_BLOCK_UP       = "TextEditor.SelectBlockUp";
const char * const SELECT_BLOCK_DOWN     = "TextEditor.SelectBlockDown";
const char * const MOVE_LINE_UP          = "TextEditor.MoveLineUp";
const char * const MOVE_LINE_DOWN        = "TextEditor.MoveLineDown";
const char * const COPY_LINE_UP          = "TextEditor.CopyLineUp";
const char * const COPY_LINE_DOWN        = "TextEditor.CopyLineDown";
const char * const JOIN_LINES            = "TextEditor.JoinLines";
const char * const CUT_LINE              = "TextEditor.CutLine";
const char * const DELETE_LINE           = "TextEditor.DeleteLine";
const char * const DELETE_WORD           = "TextEditor.DeleteWord";
const char * const SELECT_ENCODING       = "TextEditor.SelectEncoding";
const char * const REWRAP_PARAGRAPH       =  "TextEditor.RewrapParagraph";
const char * const GOTO_OPENING_PARENTHESIS = "TextEditor.GotoOpeningParenthesis";
const char * const GOTO_CLOSING_PARENTHESIS = "TextEditor.GotoClosingParenthesis";
const char * const C_TEXTEDITOR_MIMETYPE_TEXT = "text/plain";
const char * const C_TEXTEDITOR_MIMETYPE_XML = "application/xml";


// Text color and style categories
const char * const C_TEXT                = "Text";

const char * const C_LINK                = "Link";
const char * const C_SELECTION           = "Selection";
const char * const C_LINE_NUMBER         = "LineNumber";
const char * const C_SEARCH_RESULT       = "SearchResult";
const char * const C_SEARCH_SCOPE        = "SearchScope";
const char * const C_PARENTHESES         = "Parentheses";
const char * const C_CURRENT_LINE        = "CurrentLine";
const char * const C_CURRENT_LINE_NUMBER = "CurrentLineNumber";
const char * const C_OCCURRENCES         = "Occurrences";
const char * const C_OCCURRENCES_UNUSED  = "Occurrences.Unused";
const char * const C_OCCURRENCES_RENAME  = "Occurrences.Rename";

const char * const C_NUMBER              = "Number";
const char * const C_STRING              = "String";
const char * const C_TYPE                = "Type";
const char * const C_KEYWORD             = "Keyword";
const char * const C_OPERATOR            = "Operator";
const char * const C_PREPROCESSOR        = "Preprocessor";
const char * const C_LABEL               = "Label";
const char * const C_COMMENT             = "Comment";
const char * const C_DOXYGEN_COMMENT     = "Doxygen.Comment";
const char * const C_DOXYGEN_TAG         = "Doxygen.Tag";
const char * const C_VISUAL_WHITESPACE   = "VisualWhitespace";

const char * const C_DISABLED_CODE       = "DisabledCode";

const char * const C_ADDED_LINE          = "AddedLine";
const char * const C_REMOVED_LINE        = "RemovedLine";
const char * const C_DIFF_FILE           = "DiffFile";
const char * const C_DIFF_LOCATION       = "DiffLocation";

const char * const TEXT_EDITOR_SETTINGS_CATEGORY = "C.TextEditor";
const char * const TEXT_EDITOR_SETTINGS_CATEGORY_ICON = ":/core/images/category_texteditor.png";
const char * const TEXT_EDITOR_SETTINGS_TR_CATEGORY = QT_TRANSLATE_NOOP("TextEditor", "Text Editor");

} // namespace Constants
} // namespace TextEditor

#endif // TEXTEDITORCONSTANTS_H
