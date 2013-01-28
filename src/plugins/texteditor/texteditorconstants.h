/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef TEXTEDITORCONSTANTS_H
#define TEXTEDITORCONSTANTS_H

#include <QtGlobal>

namespace TextEditor {

// Text color and style categories
enum TextStyle {
    C_TEXT,

    C_LINK,
    C_SELECTION,
    C_LINE_NUMBER,
    C_SEARCH_RESULT,
    C_SEARCH_SCOPE,
    C_PARENTHESES,
    C_CURRENT_LINE,
    C_CURRENT_LINE_NUMBER,
    C_OCCURRENCES,
    C_OCCURRENCES_UNUSED,
    C_OCCURRENCES_RENAME,

    C_NUMBER,
    C_STRING,
    C_TYPE,
    C_LOCAL,
    C_FIELD,
    C_ENUMERATION,
    C_VIRTUAL_METHOD,
    C_FUNCTION,
    C_KEYWORD,
    C_OPERATOR,
    C_PREPROCESSOR,
    C_LABEL,
    C_COMMENT,
    C_DOXYGEN_COMMENT,
    C_DOXYGEN_TAG,
    C_VISUAL_WHITESPACE,
    C_QML_LOCAL_ID,
    C_QML_EXTERNAL_ID,
    C_QML_TYPE_ID,
    C_QML_ROOT_OBJECT_PROPERTY,
    C_QML_SCOPE_OBJECT_PROPERTY,
    C_QML_EXTERNAL_OBJECT_PROPERTY,
    C_JS_SCOPE_VAR,
    C_JS_IMPORT_VAR,
    C_JS_GLOBAL_VAR,
    C_QML_STATE_NAME,
    C_BINDING,


    C_DISABLED_CODE,

    C_ADDED_LINE,
    C_REMOVED_LINE,
    C_DIFF_FILE,
    C_DIFF_LOCATION,

    C_LAST_STYLE_SENTINEL
};

namespace Constants {

const char C_TEXTEDITOR[]          = "Text Editor";
const char COMPLETE_THIS[]         = "TextEditor.CompleteThis";
const char QUICKFIX_THIS[]         = "TextEditor.QuickFix";
const char VISUALIZE_WHITESPACE[]  = "TextEditor.VisualizeWhitespace";
const char CLEAN_WHITESPACE[]      = "TextEditor.CleanWhitespace";
const char TEXT_WRAPPING[]         = "TextEditor.TextWrapping";
const char UN_COMMENT_SELECTION[]  = "TextEditor.UnCommentSelection";
const char FOLD[]                  = "TextEditor.Fold";
const char UNFOLD[]                = "TextEditor.Unfold";
const char UNFOLD_ALL[]            = "TextEditor.UnFoldAll";
const char AUTO_INDENT_SELECTION[] = "TextEditor.AutoIndentSelection";
const char INCREASE_FONT_SIZE[]    = "TextEditor.IncreaseFontSize";
const char DECREASE_FONT_SIZE[]    = "TextEditor.DecreaseFontSize";
const char RESET_FONT_SIZE[]       = "TextEditor.ResetFontSize";
const char GOTO_BLOCK_START[]      = "TextEditor.GotoBlockStart";
const char GOTO_BLOCK_START_WITH_SELECTION[] = "TextEditor.GotoBlockStartWithSelection";
const char GOTO_BLOCK_END[]        = "TextEditor.GotoBlockEnd";
const char GOTO_BLOCK_END_WITH_SELECTION[] = "TextEditor.GotoBlockEndWithSelection";
const char SELECT_BLOCK_UP[]       = "TextEditor.SelectBlockUp";
const char SELECT_BLOCK_DOWN[]     = "TextEditor.SelectBlockDown";
const char MOVE_LINE_UP[]          = "TextEditor.MoveLineUp";
const char MOVE_LINE_DOWN[]        = "TextEditor.MoveLineDown";
const char COPY_LINE_UP[]          = "TextEditor.CopyLineUp";
const char COPY_LINE_DOWN[]        = "TextEditor.CopyLineDown";
const char JOIN_LINES[]            = "TextEditor.JoinLines";
const char INSERT_LINE_ABOVE[]     = "TextEditor.InsertLineAboveCurrentLine";
const char INSERT_LINE_BELOW[]     = "TextEditor.InsertLineBelowCurrentLine";
const char UPPERCASE_SELECTION[]   = "TextEditor.UppercaseSelection";
const char LOWERCASE_SELECTION[]   = "TextEditor.LowercaseSelection";
const char CUT_LINE[]              = "TextEditor.CutLine";
const char COPY_LINE[]             = "TextEditor.CopyLine";
const char DELETE_LINE[]           = "TextEditor.DeleteLine";
const char DELETE_END_OF_WORD[]    = "TextEditor.DeleteEndOfWord";
const char DELETE_END_OF_WORD_CAMEL_CASE[] = "TextEditor.DeleteEndOfWordCamelCase";
const char DELETE_START_OF_WORD[]  = "TextEditor.DeleteStartOfWord";
const char DELETE_START_OF_WORD_CAMEL_CASE[] = "TextEditor.DeleteStartOfWordCamelCase";
const char SELECT_ENCODING[]       = "TextEditor.SelectEncoding";
const char REWRAP_PARAGRAPH[]      =  "TextEditor.RewrapParagraph";
const char GOTO_LINE_START[]       = "TextEditor.GotoLineStart";
const char GOTO_LINE_END[]         = "TextEditor.GotoLineEnd";
const char GOTO_NEXT_LINE[]        = "TextEditor.GotoNextLine";
const char GOTO_PREVIOUS_LINE[]    = "TextEditor.GotoPreviousLine";
const char GOTO_PREVIOUS_CHARACTER[] = "TextEditor.GotoPreviousCharacter";
const char GOTO_NEXT_CHARACTER[]   = "TextEditor.GotoNextCharacter";
const char GOTO_PREVIOUS_WORD[]    = "TextEditor.GotoPreviousWord";
const char GOTO_NEXT_WORD[]        = "TextEditor.GotoNextWord";
const char GOTO_PREVIOUS_WORD_CAMEL_CASE[] = "TextEditor.GotoPreviousWordCamelCase";
const char GOTO_NEXT_WORD_CAMEL_CASE[] = "TextEditor.GotoNextWordCamelCase";
const char GOTO_LINE_START_WITH_SELECTION[] = "TextEditor.GotoLineStartWithSelection";
const char GOTO_LINE_END_WITH_SELECTION[] = "TextEditor.GotoLineEndWithSelection";
const char GOTO_NEXT_LINE_WITH_SELECTION[] = "TextEditor.GotoNextLineWithSelection";
const char GOTO_PREVIOUS_LINE_WITH_SELECTION[] = "TextEditor.GotoPreviousLineWithSelection";
const char GOTO_PREVIOUS_CHARACTER_WITH_SELECTION[] = "TextEditor.GotoPreviousCharacterWithSelection";
const char GOTO_NEXT_CHARACTER_WITH_SELECTION[] = "TextEditor.GotoNextCharacterWithSelection";
const char GOTO_PREVIOUS_WORD_WITH_SELECTION[] = "TextEditor.GotoPreviousWordWithSelection";
const char GOTO_NEXT_WORD_WITH_SELECTION[] = "TextEditor.GotoNextWordWithSelection";
const char GOTO_PREVIOUS_WORD_CAMEL_CASE_WITH_SELECTION[] = "TextEditor.GotoPreviousWordCamelCaseWithSelection";
const char GOTO_NEXT_WORD_CAMEL_CASE_WITH_SELECTION[] = "TextEditor.GotoNextWordCamelCaseWithSelection";
const char C_TEXTEDITOR_MIMETYPE_TEXT[] = "text/plain";
const char INFO_SYNTAX_DEFINITION[] = "TextEditor.InfoSyntaxDefinition";
const char TASK_DOWNLOAD_DEFINITIONS[] = "TextEditor.Task.Download";
const char TASK_REGISTER_DEFINITIONS[] = "TextEditor.Task.Register";
const char TASK_OPEN_FILE[]        = "TextEditor.Task.OpenFile";
const char CIRCULAR_PASTE[]        = "TextEditor.CircularPaste";
const char SWITCH_UTF8BOM[]        = "TextEditor.SwitchUtf8bom";
const char INDENT[]        = "TextEditor.Indent";
const char UNINDENT[]        = "TextEditor.Unindent";
const char FOLLOW_SYMBOL_UNDER_CURSOR[] = "TextEditor.FollowSymbolUnderCursor";
const char JUMP_TO_FILE_UNDER_CURSOR[] = "TextEditor.JumpToFileUnderCursor";

const char *nameForStyle(TextStyle style);
TextStyle styleFromName(const char *name);

const char TEXT_EDITOR_SETTINGS_CATEGORY[] = "C.TextEditor";
const char TEXT_EDITOR_SETTINGS_CATEGORY_ICON[] = ":/core/images/category_texteditor.png";
const char TEXT_EDITOR_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("TextEditor", "Text Editor");
const char TEXT_EDITOR_FONT_SETTINGS[] = "A.FontSettings";
const char TEXT_EDITOR_BEHAVIOR_SETTINGS[] = "B.BehaviourSettings";
const char TEXT_EDITOR_DISPLAY_SETTINGS[] = "D.DisplaySettings";
const char TEXT_EDITOR_HIGHLIGHTER_SETTINGS[] = "E.HighlighterSettings";
const char TEXT_EDITOR_SNIPPETS_SETTINGS[] = "F.SnippetsSettings";

const char SNIPPET_EDITOR_ID[]     = "TextEditor.SnippetEditor";
const char TEXT_SNIPPET_GROUP_ID[] = "Text";

const char GLOBAL_SETTINGS_ID[]    = "Global";

/**
 * Delay before tooltip will be shown near completion assistant proposal
 */
const unsigned COMPLETION_ASSIST_TOOLTIP_DELAY = 100;

} // namespace Constants
} // namespace TextEditor

#endif // TEXTEDITORCONSTANTS_H
