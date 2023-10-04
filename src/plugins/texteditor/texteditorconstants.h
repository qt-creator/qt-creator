// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QtGlobal>

namespace TextEditor {

// Text color and style categories
enum TextStyle : quint8 {
    C_TEXT,

    C_LINK,
    C_SELECTION,
    C_LINE_NUMBER,
    C_SEARCH_RESULT,
    C_SEARCH_RESULT_ALT1,
    C_SEARCH_RESULT_ALT2,
    C_SEARCH_RESULT_CONTAINING_FUNCTION,
    C_SEARCH_SCOPE,
    C_PARENTHESES,
    C_PARENTHESES_MISMATCH,
    C_AUTOCOMPLETE,
    C_CURRENT_LINE,
    C_CURRENT_LINE_NUMBER,
    C_OCCURRENCES,
    C_OCCURRENCES_UNUSED,
    C_OCCURRENCES_RENAME,

    C_NUMBER,
    C_STRING,
    C_TYPE,
    C_CONCEPT,
    C_NAMESPACE,
    C_LOCAL,
    C_PARAMETER,
    C_GLOBAL,
    C_FIELD,
    C_ENUMERATION,
    C_VIRTUAL_METHOD,
    C_FUNCTION,
    C_KEYWORD,
    C_PRIMITIVE_TYPE,
    C_OPERATOR,
    C_OVERLOADED_OPERATOR,
    C_PUNCTUATION,
    C_PREPROCESSOR,
    C_MACRO,
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

    C_DIFF_FILE_LINE,
    C_DIFF_CONTEXT_LINE,
    C_DIFF_SOURCE_LINE,
    C_DIFF_SOURCE_CHAR,
    C_DIFF_DEST_LINE,
    C_DIFF_DEST_CHAR,

    C_LOG_CHANGE_LINE,
    C_LOG_AUTHOR_NAME,
    C_LOG_COMMIT_DATE,
    C_LOG_COMMIT_HASH,
    C_LOG_COMMIT_SUBJECT,
    C_LOG_DECORATION,

    C_WARNING,
    C_WARNING_CONTEXT,
    C_ERROR,
    C_ERROR_CONTEXT,

    C_DECLARATION,
    C_FUNCTION_DEFINITION,
    C_OUTPUT_ARGUMENT,
    C_STATIC_MEMBER,

    C_COCO_CODE_ADDED,
    C_COCO_PARTIALLY_COVERED,
    C_COCO_NOT_COVERED,
    C_COCO_FULLY_COVERED,
    C_COCO_MANUALLY_VALIDATED,
    C_COCO_DEAD_CODE,
    C_COCO_EXECUTION_COUNT_TOO_LOW,
    C_COCO_NOT_COVERED_INFO,
    C_COCO_COVERED_INFO,
    C_COCO_MANUALLY_VALIDATED_INFO,

    C_LAST_STYLE_SENTINEL
};

namespace Constants {

const char C_TEXTEDITOR[]          = "Text Editor";
const char M_STANDARDCONTEXTMENU[] = "TextEditor.StandardContextMenu";
const char G_UNDOREDO[]            = "TextEditor.UndoRedoGroup";
const char G_COPYPASTE[]           = "TextEditor.CopyPasteGroup";
const char G_SELECT[]              = "TextEditor.SelectGroup";
const char G_BOM[]                 = "TextEditor.BomGroup";
const char COMPLETE_THIS[]         = "TextEditor.CompleteThis";
const char FUNCTION_HINT[]         = "TextEditor.FunctionHint";
const char QUICKFIX_THIS[]         = "TextEditor.QuickFix";
const char SHOWCONTEXTMENU[]       = "TextEditor.ShowContextMenu";
const char CREATE_SCRATCH_BUFFER[] = "TextEditor.CreateScratchBuffer";
const char VISUALIZE_WHITESPACE[]  = "TextEditor.VisualizeWhitespace";
const char CLEAN_WHITESPACE[]      = "TextEditor.CleanWhitespace";
const char TEXT_WRAPPING[]         = "TextEditor.TextWrapping";
const char UN_COMMENT_SELECTION[]  = "TextEditor.UnCommentSelection";
const char FOLD[]                  = "TextEditor.Fold";
const char UNFOLD[]                = "TextEditor.Unfold";
const char UNFOLD_ALL[]            = "TextEditor.UnFoldAll";
const char AUTO_INDENT_SELECTION[] = "TextEditor.AutoIndentSelection";
const char AUTO_FORMAT_SELECTION[] = "TextEditor.AutoFormatSelection";
const char INCREASE_FONT_SIZE[]    = "TextEditor.IncreaseFontSize";
const char DECREASE_FONT_SIZE[]    = "TextEditor.DecreaseFontSize";
const char RESET_FONT_SIZE[]       = "TextEditor.ResetFontSize";
const char GOTO_BLOCK_START[]      = "TextEditor.GotoBlockStart";
const char GOTO_BLOCK_START_WITH_SELECTION[] = "TextEditor.GotoBlockStartWithSelection";
const char GOTO_BLOCK_END[]        = "TextEditor.GotoBlockEnd";
const char GOTO_BLOCK_END_WITH_SELECTION[] = "TextEditor.GotoBlockEndWithSelection";
const char SELECT_BLOCK_UP[]       = "TextEditor.SelectBlockUp";
const char SELECT_BLOCK_DOWN[]     = "TextEditor.SelectBlockDown";
const char SELECT_WORD_UNDER_CURSOR[]   = "TextEditor.SelectWordUnderCursor";
const char VIEW_PAGE_UP[] = "TextEditor.viewPageUp";
const char VIEW_PAGE_DOWN[] = "TextEditor.viewPageDown";
const char VIEW_LINE_UP[] = "TextEditor.viewLineUp";
const char VIEW_LINE_DOWN[] = "TextEditor.viewLineDown";
const char MOVE_LINE_UP[]          = "TextEditor.MoveLineUp";
const char MOVE_LINE_DOWN[]        = "TextEditor.MoveLineDown";
const char COPY_LINE_UP[]          = "TextEditor.CopyLineUp";
const char COPY_LINE_DOWN[]        = "TextEditor.CopyLineDown";
const char COPY_WITH_HTML[]        = "TextEditor.CopyWithHtml";
const char JOIN_LINES[]            = "TextEditor.JoinLines";
const char INSERT_LINE_ABOVE[]     = "TextEditor.InsertLineAboveCurrentLine";
const char INSERT_LINE_BELOW[]     = "TextEditor.InsertLineBelowCurrentLine";
const char UPPERCASE_SELECTION[]   = "TextEditor.UppercaseSelection";
const char LOWERCASE_SELECTION[]   = "TextEditor.LowercaseSelection";
const char SORT_SELECTED_LINES[]   = "TextEditor.SortSelectedLines";
const char CUT_LINE[]              = "TextEditor.CutLine";
const char COPY_LINE[]             = "TextEditor.CopyLine";
const char ADD_SELECT_NEXT_FIND_MATCH[] = "TextEditor.AddSelectionNextFindMatch";
const char ADD_CURSORS_TO_LINE_ENDS[] = "TextEditor.AddCursorsAtLineEnd";
const char DUPLICATE_SELECTION[]   = "TextEditor.DuplicateSelection";
const char DUPLICATE_SELECTION_AND_COMMENT[] = "TextEditor.DuplicateSelectionAndComment";
const char DELETE_LINE[]           = "TextEditor.DeleteLine";
const char DELETE_END_OF_WORD[]    = "TextEditor.DeleteEndOfWord";
const char DELETE_END_OF_LINE[]    = "TextEditor.DeleteEndOfLine";
const char DELETE_END_OF_WORD_CAMEL_CASE[] = "TextEditor.DeleteEndOfWordCamelCase";
const char DELETE_START_OF_WORD[]  = "TextEditor.DeleteStartOfWord";
const char DELETE_START_OF_LINE[]  = "TextEditor.DeleteStartOfLine";
const char DELETE_START_OF_WORD_CAMEL_CASE[] = "TextEditor.DeleteStartOfWordCamelCase";
const char SELECT_ENCODING[]       = "TextEditor.SelectEncoding";
const char REWRAP_PARAGRAPH[]      =  "TextEditor.RewrapParagraph";
const char GOTO_DOCUMENT_START[]   = "TextEditor.GotoDocumentStart";
const char GOTO_DOCUMENT_END[]     = "TextEditor.GotoDocumentEnd";
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
const char INFO_MISSING_SYNTAX_DEFINITION[] = "TextEditor.InfoSyntaxDefinition";
const char INFO_MULTIPLE_SYNTAX_DEFINITIONS[] = "TextEditor.InfoMultipleSyntaxDefinitions";
const char TASK_OPEN_FILE[]        = "TextEditor.Task.OpenFile";
const char CIRCULAR_PASTE[]        = "TextEditor.CircularPaste";
const char NO_FORMAT_PASTE[]       = "TextEditor.NoFormatPaste";
const char SWITCH_UTF8BOM[]        = "TextEditor.SwitchUtf8bom";
const char INDENT[]        = "TextEditor.Indent";
const char UNINDENT[]        = "TextEditor.Unindent";
const char FOLLOW_SYMBOL_UNDER_CURSOR[] = "TextEditor.FollowSymbolUnderCursor";
const char FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT[] = "TextEditor.FollowSymbolUnderCursorInNextSplit";
const char FOLLOW_SYMBOL_TO_TYPE[] = "TextEditor.FollowSymbolToType";
const char FOLLOW_SYMBOL_TO_TYPE_IN_NEXT_SPLIT[] = "TextEditor.FollowSymbolToTypeInNextSplit";
const char FIND_USAGES[] = "TextEditor.FindUsages";
// moved from CppEditor to TextEditor avoid breaking the setting by using the old key
const char RENAME_SYMBOL[] = "CppEditor.RenameSymbolUnderCursor";
const char OPEN_CALL_HIERARCHY[] = "TextEditor.OpenCallHierarchy";
const char JUMP_TO_FILE_UNDER_CURSOR[] = "TextEditor.JumpToFileUnderCursor";
const char JUMP_TO_FILE_UNDER_CURSOR_IN_NEXT_SPLIT[] = "TextEditor.JumpToFileUnderCursorInNextSplit";

const char SCROLL_BAR_SEARCH_RESULT[] = "TextEditor.ScrollBarSearchResult";
const char SCROLL_BAR_CURRENT_LINE[] = "TextEditor.ScrollBarCurrentLine";

const TEXTEDITOR_EXPORT char *nameForStyle(TextStyle style);
TextStyle styleFromName(const char *name);

const char TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH[] =  ":/texteditor/images/settingscategory_texteditor.png";
const char TEXT_EDITOR_SETTINGS_CATEGORY[] = "C.TextEditor";
const char TEXT_EDITOR_FONT_SETTINGS[] = "A.FontSettings";
const char TEXT_EDITOR_BEHAVIOR_SETTINGS[] = "B.BehaviourSettings";
const char TEXT_EDITOR_DISPLAY_SETTINGS[] = "D.DisplaySettings";
const char TEXT_EDITOR_HIGHLIGHTER_SETTINGS[] = "E.HighlighterSettings";
const char TEXT_EDITOR_SNIPPETS_SETTINGS[] = "F.SnippetsSettings";
const char TEXT_EDITOR_COMMENTS_SETTINGS[] = "Q.CommentsSettings";

const char HIGHLIGHTER_SETTINGS_CATEGORY[] = "HighlighterSettings";

const char SNIPPET_EDITOR_ID[]     = "TextEditor.SnippetEditor";
const char TEXT_SNIPPET_GROUP_ID[] = "Text";

const char GLOBAL_SETTINGS_ID[]    = "Global";
const char GENERIC_PROPOSAL_ID[] = "TextEditor.GenericProposalId";

const char BOOKMARKS_PREV_ACTION[]        = "Bookmarks.Previous";
const char BOOKMARKS_NEXT_ACTION[]        = "Bookmarks.Next";

/**
 * Delay before tooltip will be shown near completion assistant proposal
 */
const unsigned COMPLETION_ASSIST_TOOLTIP_DELAY = 100;

} // namespace Constants
} // namespace TextEditor
