/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TEXTEDITORCONSTANTS_H
#define TEXTEDITORCONSTANTS_H

#include <QtCore/QtGlobal>

namespace TextEditor {
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

// Text color and style categories
const char C_TEXT[]                = "Text";

const char C_LINK[]                = "Link";
const char C_SELECTION[]           = "Selection";
const char C_LINE_NUMBER[]         = "LineNumber";
const char C_SEARCH_RESULT[]       = "SearchResult";
const char C_SEARCH_SCOPE[]        = "SearchScope";
const char C_PARENTHESES[]         = "Parentheses";
const char C_CURRENT_LINE[]        = "CurrentLine";
const char C_CURRENT_LINE_NUMBER[] = "CurrentLineNumber";
const char C_OCCURRENCES[]         = "Occurrences";
const char C_OCCURRENCES_UNUSED[]  = "Occurrences.Unused";
const char C_OCCURRENCES_RENAME[]  = "Occurrences.Rename";

const char C_NUMBER[]              = "Number";
const char C_STRING[]              = "String";
const char C_TYPE[]                = "Type";
const char C_LOCAL[]               = "Local";
const char C_FIELD[]               = "Field";
const char C_STATIC[]              = "Static";
const char C_VIRTUAL_METHOD[]      = "VirtualMethod";
const char C_KEYWORD[]             = "Keyword";
const char C_OPERATOR[]            = "Operator";
const char C_PREPROCESSOR[]        = "Preprocessor";
const char C_LABEL[]               = "Label";
const char C_COMMENT[]             = "Comment";
const char C_DOXYGEN_COMMENT[]     = "Doxygen.Comment";
const char C_DOXYGEN_TAG[]         = "Doxygen.Tag";
const char C_VISUAL_WHITESPACE[]   = "VisualWhitespace";
const char C_QML_LOCAL_ID[]        = "QmlLocalId";
const char C_QML_EXTERNAL_ID[]     = "QmlExternalId";
const char C_QML_TYPE_ID[]         = "QmlTypeId";
const char C_QML_ROOT_OBJECT_PROPERTY[]     = "QmlRootObjectProperty";
const char C_QML_SCOPE_OBJECT_PROPERTY[]    = "QmlScopeObjectProperty";
const char C_QML_EXTERNAL_OBJECT_PROPERTY[] = "QmlExternalObjectProperty";
const char C_JS_SCOPE_VAR[]        = "JsScopeVar";
const char C_JS_IMPORT_VAR[]       = "JsImportVar";
const char C_JS_GLOBAL_VAR[]       = "JsGlobalVar";
const char C_QML_STATE_NAME[]      = "QmlStateName";
const char C_BINDING[]             = "Binding";


const char C_DISABLED_CODE[]       = "DisabledCode";

const char C_ADDED_LINE[]          = "AddedLine";
const char C_REMOVED_LINE[]        = "RemovedLine";
const char C_DIFF_FILE[]           = "DiffFile";
const char C_DIFF_LOCATION[]       = "DiffLocation";

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

} // namespace Constants
} // namespace TextEditor

#endif // TEXTEDITORCONSTANTS_H
