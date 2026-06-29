// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QLoggingCategory>

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
    C_ATTRIBUTE,
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

    C_INFO,
    C_INFO_CONTEXT,
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

inline constexpr char C_TEXTEDITOR[]          = "Text Editor";
inline constexpr char M_STANDARDCONTEXTMENU[] = "TextEditor.StandardContextMenu";
inline constexpr char G_UNDOREDO[]            = "TextEditor.UndoRedoGroup";
inline constexpr char G_COPYPASTE[]           = "TextEditor.CopyPasteGroup";
inline constexpr char G_SELECT[]              = "TextEditor.SelectGroup";
inline constexpr char G_BOM[]                 = "TextEditor.BomGroup";
inline constexpr char COMPLETE_THIS[]         = "TextEditor.CompleteThis";
inline constexpr char FUNCTION_HINT[]         = "TextEditor.FunctionHint";
inline constexpr char QUICKFIX_THIS[]         = "TextEditor.QuickFix";
inline constexpr char SHOWCONTEXTMENU[]       = "TextEditor.ShowContextMenu";
inline constexpr char CREATE_SCRATCH_BUFFER[] = "TextEditor.CreateScratchBuffer";
inline constexpr char VISUALIZE_WHITESPACE[]  = "TextEditor.VisualizeWhitespace";
inline constexpr char CLEAN_WHITESPACE[]      = "TextEditor.CleanWhitespace";
inline constexpr char TEXT_WRAPPING[]         = "TextEditor.TextWrapping";
inline constexpr char UN_COMMENT_SELECTION[]  = "TextEditor.UnCommentSelection";
inline constexpr char FOLD[]                  = "TextEditor.Fold";
inline constexpr char UNFOLD[]                = "TextEditor.Unfold";
inline constexpr char FOLD_RECURSIVELY[]      = "TextEditor.FoldRecursively";
inline constexpr char UNFOLD_RECURSIVELY[]    = "TextEditor.UnfoldRecursively";
inline constexpr char UNFOLD_ALL[]            = "TextEditor.UnFoldAll";
inline constexpr char AUTO_INDENT_SELECTION[] = "TextEditor.AutoIndentSelection";
inline constexpr char AUTO_FORMAT_SELECTION[] = "TextEditor.AutoFormatSelection";
inline constexpr char REFORMAT_FILE[]         = "TextEditor.ReformatFile";
inline constexpr char GOTO_BLOCK_START[]      = "TextEditor.GotoBlockStart";
inline constexpr char GOTO_BLOCK_START_WITH_SELECTION[] = "TextEditor.GotoBlockStartWithSelection";
inline constexpr char GOTO_BLOCK_END[]        = "TextEditor.GotoBlockEnd";
inline constexpr char GOTO_BLOCK_END_WITH_SELECTION[] = "TextEditor.GotoBlockEndWithSelection";
inline constexpr char SELECT_BLOCK_UP[]       = "TextEditor.SelectBlockUp";
inline constexpr char SELECT_BLOCK_DOWN[]     = "TextEditor.SelectBlockDown";
inline constexpr char SELECT_WORD_UNDER_CURSOR[]   = "TextEditor.SelectWordUnderCursor";
inline constexpr char CLEAR_SELECTION[]       = "TextEditor.ClearSelection";
inline constexpr char VIEW_PAGE_UP[] = "TextEditor.viewPageUp";
inline constexpr char VIEW_PAGE_DOWN[] = "TextEditor.viewPageDown";
inline constexpr char VIEW_LINE_UP[] = "TextEditor.viewLineUp";
inline constexpr char VIEW_LINE_DOWN[] = "TextEditor.viewLineDown";
inline constexpr char MOVE_LINE_UP[]          = "TextEditor.MoveLineUp";
inline constexpr char MOVE_LINE_DOWN[]        = "TextEditor.MoveLineDown";
inline constexpr char COPY_LINE_UP[]          = "TextEditor.CopyLineUp";
inline constexpr char COPY_LINE_DOWN[]        = "TextEditor.CopyLineDown";
inline constexpr char COPY_WITH_HTML[]        = "TextEditor.CopyWithHtml";
inline constexpr char JOIN_LINES[]            = "TextEditor.JoinLines";
inline constexpr char INSERT_LINE_ABOVE[]     = "TextEditor.InsertLineAboveCurrentLine";
inline constexpr char INSERT_LINE_BELOW[]     = "TextEditor.InsertLineBelowCurrentLine";
inline constexpr char UPPERCASE_SELECTION[]   = "TextEditor.UppercaseSelection";
inline constexpr char LOWERCASE_SELECTION[]   = "TextEditor.LowercaseSelection";
inline constexpr char SORT_LINES[]            = "TextEditor.SortSelectedLines";
inline constexpr char CUT_LINE[]              = "TextEditor.CutLine";
inline constexpr char COPY_LINE[]             = "TextEditor.CopyLine";
inline constexpr char ADD_SELECT_NEXT_FIND_MATCH[] = "TextEditor.AddSelectionNextFindMatch";
inline constexpr char ADD_CURSORS_TO_LINE_ENDS[] = "TextEditor.AddCursorsAtLineEnd";
inline constexpr char DUPLICATE_SELECTION[]   = "TextEditor.DuplicateSelection";
inline constexpr char DUPLICATE_SELECTION_AND_COMMENT[] = "TextEditor.DuplicateSelectionAndComment";
inline constexpr char DELETE_LINE[]           = "TextEditor.DeleteLine";
inline constexpr char DELETE_END_OF_WORD[]    = "TextEditor.DeleteEndOfWord";
inline constexpr char DELETE_END_OF_LINE[]    = "TextEditor.DeleteEndOfLine";
inline constexpr char DELETE_END_OF_WORD_CAMEL_CASE[] = "TextEditor.DeleteEndOfWordCamelCase";
inline constexpr char DELETE_START_OF_WORD[]  = "TextEditor.DeleteStartOfWord";
inline constexpr char DELETE_START_OF_LINE[]  = "TextEditor.DeleteStartOfLine";
inline constexpr char DELETE_START_OF_WORD_CAMEL_CASE[] = "TextEditor.DeleteStartOfWordCamelCase";
inline constexpr char SELECT_ENCODING[]       = "TextEditor.SelectEncoding";
inline constexpr char REWRAP_PARAGRAPH[]      =  "TextEditor.RewrapParagraph";
inline constexpr char GOTO_DOCUMENT_START[]   = "TextEditor.GotoDocumentStart";
inline constexpr char GOTO_DOCUMENT_END[]     = "TextEditor.GotoDocumentEnd";
inline constexpr char GOTO_LINE_START[]       = "TextEditor.GotoLineStart";
inline constexpr char GOTO_LINE_END[]         = "TextEditor.GotoLineEnd";
inline constexpr char GOTO_NEXT_LINE[]        = "TextEditor.GotoNextLine";
inline constexpr char GOTO_PREVIOUS_LINE[]    = "TextEditor.GotoPreviousLine";
inline constexpr char GOTO_PREVIOUS_CHARACTER[] = "TextEditor.GotoPreviousCharacter";
inline constexpr char GOTO_NEXT_CHARACTER[]   = "TextEditor.GotoNextCharacter";
inline constexpr char GOTO_PREVIOUS_WORD[]    = "TextEditor.GotoPreviousWord";
inline constexpr char GOTO_NEXT_WORD[]        = "TextEditor.GotoNextWord";
inline constexpr char GOTO_PREVIOUS_WORD_CAMEL_CASE[] = "TextEditor.GotoPreviousWordCamelCase";
inline constexpr char GOTO_NEXT_WORD_CAMEL_CASE[] = "TextEditor.GotoNextWordCamelCase";
inline constexpr char GOTO_LINE_START_WITH_SELECTION[] = "TextEditor.GotoLineStartWithSelection";
inline constexpr char GOTO_LINE_END_WITH_SELECTION[] = "TextEditor.GotoLineEndWithSelection";
inline constexpr char GOTO_NEXT_LINE_WITH_SELECTION[] = "TextEditor.GotoNextLineWithSelection";
inline constexpr char GOTO_PREVIOUS_LINE_WITH_SELECTION[] = "TextEditor.GotoPreviousLineWithSelection";
inline constexpr char GOTO_PREVIOUS_CHARACTER_WITH_SELECTION[] = "TextEditor.GotoPreviousCharacterWithSelection";
inline constexpr char GOTO_NEXT_CHARACTER_WITH_SELECTION[] = "TextEditor.GotoNextCharacterWithSelection";
inline constexpr char GOTO_PREVIOUS_WORD_WITH_SELECTION[] = "TextEditor.GotoPreviousWordWithSelection";
inline constexpr char GOTO_NEXT_WORD_WITH_SELECTION[] = "TextEditor.GotoNextWordWithSelection";
inline constexpr char GOTO_PREVIOUS_WORD_CAMEL_CASE_WITH_SELECTION[] = "TextEditor.GotoPreviousWordCamelCaseWithSelection";
inline constexpr char GOTO_NEXT_WORD_CAMEL_CASE_WITH_SELECTION[] = "TextEditor.GotoNextWordCamelCaseWithSelection";
inline constexpr char SUGGESTION_APPLY[] = "TextEditor.Suggestion.Apply";
inline constexpr char SUGGESTION_APPLY_WORD[] = "TextEditor.Suggestion.ApplyWord";
inline constexpr char SUGGESTION_APPLY_LINE[] = "TextEditor.Suggestion.ApplyLine";
inline constexpr char C_TEXTEDITOR_MIMETYPE_TEXT[] = "text/plain";
inline constexpr char INFO_MISSING_SYNTAX_DEFINITION[] = "TextEditor.InfoSyntaxDefinition";
inline constexpr char INFO_MULTIPLE_SYNTAX_DEFINITIONS[] = "TextEditor.InfoMultipleSyntaxDefinitions";
inline constexpr char TASK_OPEN_FILE[]        = "TextEditor.Task.OpenFile";
inline constexpr char CIRCULAR_PASTE[]        = "TextEditor.CircularPaste";
inline constexpr char NO_FORMAT_PASTE[]       = "TextEditor.NoFormatPaste";
inline constexpr char SWITCH_UTF8BOM[]        = "TextEditor.SwitchUtf8bom";
inline constexpr char INDENT[]        = "TextEditor.Indent";
inline constexpr char UNINDENT[]        = "TextEditor.Unindent";
inline constexpr char FOLLOW_SYMBOL_UNDER_CURSOR[] = "TextEditor.FollowSymbolUnderCursor";
inline constexpr char FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT[] = "TextEditor.FollowSymbolUnderCursorInNextSplit";
inline constexpr char FOLLOW_SYMBOL_TO_TYPE[] = "TextEditor.FollowSymbolToType";
inline constexpr char FOLLOW_SYMBOL_TO_TYPE_IN_NEXT_SPLIT[] = "TextEditor.FollowSymbolToTypeInNextSplit";
inline constexpr char FIND_USAGES[] = "TextEditor.FindUsages";
// moved from CppEditor to TextEditor avoid breaking the setting by using the old key
inline constexpr char RENAME_SYMBOL[] = "CppEditor.RenameSymbolUnderCursor";
inline constexpr char OPEN_CALL_HIERARCHY[] = "TextEditor.OpenCallHierarchy";
inline constexpr char OPEN_TYPE_HIERARCHY[] = "TextEditor.OpenTypeHierarchy";
inline constexpr char TYPE_HIERARCHY_FACTORY_ID[] = "TextEditor.TypeHierarchy";
inline constexpr char JUMP_TO_FILE_UNDER_CURSOR[] = "TextEditor.JumpToFileUnderCursor";
inline constexpr char JUMP_TO_FILE_UNDER_CURSOR_IN_NEXT_SPLIT[] = "TextEditor.JumpToFileUnderCursorInNextSplit";

inline constexpr char SCROLL_BAR_SEARCH_RESULT[] = "TextEditor.ScrollBarSearchResult";
inline constexpr char SCROLL_BAR_SELECTION[] = "TextEditor.ScrollBarSelection";
inline constexpr char SCROLL_BAR_CURRENT_LINE[] = "TextEditor.ScrollBarCurrentLine";

const TEXTEDITOR_EXPORT char *nameForStyle(TextStyle style);
TextStyle styleFromName(const char *name);

inline constexpr char TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH[] =  ":/texteditor/images/settingscategory_texteditor.png";
inline constexpr char TEXT_EDITOR_SETTINGS_CATEGORY[] = "C.TextEditor";
inline constexpr char TEXT_EDITOR_FONT_SETTINGS[] = "A.FontSettings";
inline constexpr char TEXT_EDITOR_BEHAVIOR_SETTINGS[] = "B.BehaviourSettings";
inline constexpr char TEXT_EDITOR_DISPLAY_SETTINGS[] = "D.DisplaySettings";
inline constexpr char TEXT_EDITOR_HIGHLIGHTER_SETTINGS[] = "E.HighlighterSettings";
inline constexpr char TEXT_EDITOR_SNIPPETS_SETTINGS[] = "F.SnippetsSettings";
inline constexpr char TEXT_EDITOR_COMMENTS_SETTINGS[] = "Q.CommentsSettings";

inline constexpr char HIGHLIGHTER_SETTINGS_CATEGORY[] = "HighlighterSettings";

inline constexpr char SNIPPET_EDITOR_ID[]     = "TextEditor.SnippetEditor";
inline constexpr char TEXT_SNIPPET_GROUP_ID[] = "Text";

inline constexpr char GLOBAL_SETTINGS_ID[]           = "Global";
inline constexpr char CODE_STYLE_SETTINGS_PREFIX[]   = "text";
inline constexpr char GENERIC_PROPOSAL_ID[] = "TextEditor.GenericProposalId";

inline constexpr char BOOKMARKS_PREV_ACTION[]              = "Bookmarks.Previous";
inline constexpr char BOOKMARKS_NEXT_ACTION[]              = "Bookmarks.Next";
inline constexpr char BOOKMARKS_MOVEUP_ACTION[]            = "Bookmarks.MoveUp";
inline constexpr char BOOKMARKS_MOVEDOWN_ACTION[]          = "Bookmarks.MoveDown";
inline constexpr char BOOKMARKS_SORTBYFILENAMES_ACTION[]   = "Bookmarks.SortByFilenames";

/**
 * Delay before tooltip will be shown near completion assistant proposal
 */
const unsigned COMPLETION_ASSIST_TOOLTIP_DELAY = 100;

} // namespace Constants

namespace Internal { Q_DECLARE_LOGGING_CATEGORY(foldingLog) }

} // namespace TextEditor
