/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "texteditoractionhandler.h"

#include "basetexteditor.h"
#include "displaysettings.h"
#include "linenumberfilter.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"

#include <locator/locatormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QAction>

using namespace TextEditor;
using namespace TextEditor::Internal;

TextEditorActionHandler::TextEditorActionHandler(QObject *parent, Core::Id contextId,
                                                 uint optionalActions)
  : QObject(parent),
    m_undoAction(0),
    m_redoAction(0),
    m_copyAction(0),
    m_cutAction(0),
    m_pasteAction(0),
    m_circularPasteAction(0),
    m_switchUtf8bomAction(0),
    m_selectAllAction(0),
    m_gotoAction(0),
    m_printAction(0),
    m_formatAction(0),
    m_visualizeWhitespaceAction(0),
    m_cleanWhitespaceAction(0),
    m_textWrappingAction(0),
    m_unCommentSelectionAction(0),
    m_unfoldAllAction(0),
    m_foldAction(0),
    m_unfoldAction(0),
    m_cutLineAction(0),
    m_copyLineAction(0),
    m_deleteLineAction(0),
    m_deleteEndOfWordAction(0),
    m_deleteEndOfWordCamelCaseAction(0),
    m_deleteStartOfWordAction(0),
    m_deleteStartOfWordCamelCaseAction(0),
    m_selectEncodingAction(0),
    m_increaseFontSizeAction(0),
    m_decreaseFontSizeAction(0),
    m_resetFontSizeAction(0),
    m_gotoBlockStartAction(0),
    m_gotoBlockEndAction(0),
    m_gotoBlockStartWithSelectionAction(0),
    m_gotoBlockEndWithSelectionAction(0),
    m_selectBlockUpAction(0),
    m_selectBlockDownAction(0),
    m_moveLineUpAction(0),
    m_moveLineDownAction(0),
    m_copyLineUpAction(0),
    m_copyLineDownAction(0),
    m_joinLinesAction(0),
    m_insertLineAboveAction(0),
    m_insertLineBelowAction(0),
    m_upperCaseSelectionAction(0),
    m_lowerCaseSelectionAction(0),
    m_indentAction(0),
    m_unindentAction(0),
    m_followSymbolAction(0),
    m_followSymbolInNextSplitAction(0),
    m_jumpToFileAction(0),
    m_jumpToFileInNextSplitAction(0),
    m_optionalActions(optionalActions),
    m_currentEditorWidget(0),
    m_contextId(contextId)
{
    createActions();
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
        this, SLOT(updateCurrentEditor(Core::IEditor*)));
}

TextEditorActionHandler::~TextEditorActionHandler()
{
}

void TextEditorActionHandler::createActions()
{
    using namespace Core::Constants;
    using namespace TextEditor::Constants;

    m_undoAction = registerAction(UNDO,
            SLOT(undoAction()), true, tr("&Undo"));
    m_redoAction = registerAction(REDO,
            SLOT(redoAction()), true, tr("&Redo"));
    m_copyAction = registerAction(COPY,
            SLOT(copyAction()), true);
    m_cutAction = registerAction(CUT,
            SLOT(cutAction()), true);
    m_pasteAction = registerAction(PASTE,
            SLOT(pasteAction()), true);
    m_selectAllAction = registerAction(SELECTALL,
            SLOT(selectAllAction()), true);
    m_gotoAction = registerAction(GOTO,
            SLOT(gotoAction()));
    m_printAction = registerAction(PRINT,
            SLOT(printAction()));
    m_cutLineAction = registerAction(CUT_LINE,
            SLOT(cutLine()), true, tr("Cut &Line"),
            QKeySequence(tr("Shift+Del")));
    m_copyLineAction = registerAction(COPY_LINE,
            SLOT(copyLine()), false, tr("Copy &Line"),
            QKeySequence(tr("Ctrl+Ins")));
    m_deleteLineAction = registerAction(DELETE_LINE,
            SLOT(deleteLine()), true, tr("Delete &Line"));
    m_deleteEndOfWordAction = registerAction(DELETE_END_OF_WORD,
            SLOT(deleteEndOfWord()), true, tr("Delete Word from Cursor On"));
    m_deleteEndOfWordCamelCaseAction = registerAction(DELETE_END_OF_WORD_CAMEL_CASE,
            SLOT(deleteEndOfWordCamelCase()), true, tr("Delete Word Camel Case from Cursor On"));
    m_deleteStartOfWordAction = registerAction(DELETE_START_OF_WORD,
            SLOT(deleteStartOfWord()), true, tr("Delete Word up to Cursor"));
    m_deleteStartOfWordCamelCaseAction = registerAction(DELETE_START_OF_WORD_CAMEL_CASE,
            SLOT(deleteStartOfWordCamelCase()), true, tr("Delete Word Camel Case up to Cursor"));
    m_gotoBlockStartWithSelectionAction = registerAction(GOTO_BLOCK_START_WITH_SELECTION,
            SLOT(gotoBlockStartWithSelection()), true, tr("Go to Block Start with Selection"),
            QKeySequence(tr("Ctrl+{")));
    m_gotoBlockEndWithSelectionAction = registerAction(GOTO_BLOCK_END_WITH_SELECTION,
            SLOT(gotoBlockEndWithSelection()), true, tr("Go to Block End with Selection"),
            QKeySequence(tr("Ctrl+}")));
    m_moveLineUpAction = registerAction(MOVE_LINE_UP,
            SLOT(moveLineUp()), true, tr("Move Line Up"),
            QKeySequence(tr("Ctrl+Shift+Up")));
    m_moveLineDownAction = registerAction(MOVE_LINE_DOWN,
            SLOT(moveLineDown()), true, tr("Move Line Down"),
            QKeySequence(tr("Ctrl+Shift+Down")));
    m_copyLineUpAction = registerAction(COPY_LINE_UP,
            SLOT(copyLineUp()), true, tr("Copy Line Up"),
            QKeySequence(tr("Ctrl+Alt+Up")));
    m_copyLineDownAction = registerAction(COPY_LINE_DOWN,
            SLOT(copyLineDown()), true, tr("Copy Line Down"),
            QKeySequence(tr("Ctrl+Alt+Down")));
    m_joinLinesAction = registerAction(JOIN_LINES,
            SLOT(joinLines()), true, tr("Join Lines"),
            QKeySequence(tr("Ctrl+J")));
    m_insertLineAboveAction = registerAction(INSERT_LINE_ABOVE,
            SLOT(insertLineAbove()), true, tr("Insert Line Above Current Line"),
            QKeySequence(tr("Ctrl+Shift+Return")));
    m_insertLineBelowAction = registerAction(INSERT_LINE_BELOW,
            SLOT(insertLineBelow()), true, tr("Insert Line Below Current Line"),
            QKeySequence(tr("Ctrl+Return")));
    m_upperCaseSelectionAction = registerAction(UPPERCASE_SELECTION,
            SLOT(uppercaseSelection()), true, tr("Uppercase Selection"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+Shift+U") : tr("Alt+Shift+U")));
    m_lowerCaseSelectionAction = registerAction(LOWERCASE_SELECTION,
            SLOT(lowercaseSelection()), true, tr("Lowercase Selection"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+U") : tr("Alt+U")));
    m_switchUtf8bomAction = registerAction(SWITCH_UTF8BOM,
            SLOT(switchUtf8bomAction()), true);
    m_indentAction = registerAction(INDENT,
            SLOT(indent()), true, tr("Indent"));
    m_unindentAction = registerAction(UNINDENT,
            SLOT(unindent()), true, tr("Unindent"));
    m_followSymbolAction = registerAction(FOLLOW_SYMBOL_UNDER_CURSOR,
            SLOT(openLinkUnderCursor()), true, tr("Follow Symbol Under Cursor"),
            QKeySequence(Qt::Key_F2));
    m_followSymbolInNextSplitAction = registerAction(FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT,
            SLOT(openLinkUnderCursorInNextSplit()), true, tr("Follow Symbol Under Cursor in Next Split"),
            QKeySequence(Utils::HostOsInfo::isMacHost() ? tr("Meta+E, F2") : tr("Ctrl+E, F2")));
    m_jumpToFileAction = registerAction(JUMP_TO_FILE_UNDER_CURSOR,
            SLOT(openLinkUnderCursor()), true, tr("Jump To File Under Cursor"),
            QKeySequence(Qt::Key_F2));
    m_jumpToFileInNextSplitAction = registerAction(JUMP_TO_FILE_UNDER_CURSOR_IN_NEXT_SPLIT,
            SLOT(openLinkUnderCursorInNextSplit()), true,
            QKeySequence(Utils::HostOsInfo::isMacHost() ? tr("Meta+E, F2") : tr("Ctrl+E, F2")));

    // register "Edit" Menu Actions
    Core::ActionContainer *editMenu = Core::ActionManager::actionContainer(M_EDIT);
    m_selectEncodingAction = registerAction(SELECT_ENCODING,
            SLOT(selectEncoding()), false, tr("Select Encoding..."),
            QKeySequence(), G_EDIT_OTHER, editMenu);
    m_circularPasteAction = registerAction(CIRCULAR_PASTE,
            SLOT(circularPasteAction()), false, tr("Paste from Clipboard History"),
            QKeySequence(tr("Ctrl+Shift+V")), G_EDIT_COPYPASTE, editMenu);

    // register "Edit -> Advanced" Menu Actions
    Core::ActionContainer *advancedEditMenu = Core::ActionManager::actionContainer(M_EDIT_ADVANCED);
    m_formatAction = registerAction(AUTO_INDENT_SELECTION,
            SLOT(formatAction()), true, tr("Auto-&indent Selection"),
            QKeySequence(tr("Ctrl+I")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_rewrapParagraphAction = registerAction(REWRAP_PARAGRAPH,
            SLOT(rewrapParagraphAction()), true, tr("&Rewrap Paragraph"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+E, R") : tr("Ctrl+E, R")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_visualizeWhitespaceAction = registerAction(VISUALIZE_WHITESPACE,
            SLOT(setVisualizeWhitespace(bool)), false, tr("&Visualize Whitespace"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+E, Meta+V") : tr("Ctrl+E, Ctrl+V")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_visualizeWhitespaceAction->setCheckable(true);
    m_cleanWhitespaceAction = registerAction(CLEAN_WHITESPACE,
            SLOT(setTextWrapping(bool)), true, tr("Clean Whitespace"),
            QKeySequence(),
            G_EDIT_FORMAT, advancedEditMenu);
    m_textWrappingAction = registerAction(TEXT_WRAPPING,
            SLOT(setTextWrapping(bool)), false, tr("Enable Text &Wrapping"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+E, Meta+W") : tr("Ctrl+E, Ctrl+W")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_textWrappingAction->setCheckable(true);
    m_unCommentSelectionAction = registerAction(UN_COMMENT_SELECTION,
            SLOT(unCommentSelection()), true, tr("Toggle Comment &Selection"),
            QKeySequence(tr("Ctrl+/")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_foldAction = registerAction(FOLD,
            SLOT(fold()), true, tr("Fold"),
            QKeySequence(tr("Ctrl+<")),
            G_EDIT_COLLAPSING, advancedEditMenu);
    m_unfoldAction = registerAction(UNFOLD,
            SLOT(unfold()), true, tr("Unfold"),
            QKeySequence(tr("Ctrl+>")),
            G_EDIT_COLLAPSING, advancedEditMenu);
    m_unfoldAllAction = registerAction(UNFOLD_ALL,
            SLOT(unfoldAll()), true, tr("Toggle &Fold All"),
            QKeySequence(),
            G_EDIT_COLLAPSING, advancedEditMenu);
    m_increaseFontSizeAction = registerAction(INCREASE_FONT_SIZE,
            SLOT(increaseFontSize()), false, tr("Increase Font Size"),
            QKeySequence(tr("Ctrl++")),
            G_EDIT_FONT, advancedEditMenu);
    m_decreaseFontSizeAction = registerAction(DECREASE_FONT_SIZE,
            SLOT(decreaseFontSize()), false, tr("Decrease Font Size"),
            QKeySequence(tr("Ctrl+-")),
            G_EDIT_FONT, advancedEditMenu);
    m_resetFontSizeAction = registerAction(RESET_FONT_SIZE,
            SLOT(resetFontSize()), false, tr("Reset Font Size"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+0") : tr("Ctrl+0")),
            G_EDIT_FONT, advancedEditMenu);
    m_gotoBlockStartAction = registerAction(GOTO_BLOCK_START,
            SLOT(gotoBlockStart()), true, tr("Go to Block Start"),
            QKeySequence(tr("Ctrl+[")),
            G_EDIT_BLOCKS, advancedEditMenu);
    m_gotoBlockEndAction = registerAction(GOTO_BLOCK_END,
            SLOT(gotoBlockEnd()), true, tr("Go to Block End"),
            QKeySequence(tr("Ctrl+]")),
            G_EDIT_BLOCKS, advancedEditMenu);
    m_selectBlockUpAction = registerAction(SELECT_BLOCK_UP,
            SLOT(selectBlockUp()), true, tr("Select Block Up"),
            QKeySequence(tr("Ctrl+U")),
            G_EDIT_BLOCKS, advancedEditMenu);
    m_selectBlockDownAction = registerAction(SELECT_BLOCK_DOWN,
            SLOT(selectBlockDown()), true, tr("Select Block Down"),
            QKeySequence(),
            G_EDIT_BLOCKS, advancedEditMenu);

    // register GOTO Actions
    registerAction(GOTO_LINE_START,
            SLOT(gotoLineStart()), true, tr("Go to Line Start"));
    registerAction(GOTO_LINE_END,
            SLOT(gotoLineEnd()), true, tr("Go to Line End"));
    registerAction(GOTO_NEXT_LINE,
            SLOT(gotoNextLine()), true, tr("Go to Next Line"));
    registerAction(GOTO_PREVIOUS_LINE,
            SLOT(gotoPreviousLine()), true, tr("Go to Previous Line"));
    registerAction(GOTO_PREVIOUS_CHARACTER,
            SLOT(gotoPreviousCharacter()), true, tr("Go to Previous Character"));
    registerAction(GOTO_NEXT_CHARACTER,
            SLOT(gotoNextCharacter()), true, tr("Go to Next Character"));
    registerAction(GOTO_PREVIOUS_WORD,
            SLOT(gotoPreviousWord()), true, tr("Go to Previous Word"));
    registerAction(GOTO_NEXT_WORD,
            SLOT(gotoNextWord()), true, tr("Go to Next Word"));
    registerAction(GOTO_PREVIOUS_WORD_CAMEL_CASE,
            SLOT(gotoPreviousWordCamelCase()), false, tr("Go to Previous Word Camel Case"));
    registerAction(GOTO_NEXT_WORD_CAMEL_CASE,
            SLOT(gotoNextWordCamelCase()), false, tr("Go to Next Word Camel Case"));

    // register GOTO actions with selection
    registerAction(GOTO_LINE_START_WITH_SELECTION,
            SLOT(gotoLineStartWithSelection()), true, tr("Go to Line Start with Selection"));
    registerAction(GOTO_LINE_END_WITH_SELECTION,
            SLOT(gotoLineEndWithSelection()), true, tr("Go to Line End with Selection"));
    registerAction(GOTO_NEXT_LINE_WITH_SELECTION,
            SLOT(gotoNextLineWithSelection()), true, tr("Go to Next Line with Selection"));
    registerAction(GOTO_PREVIOUS_LINE_WITH_SELECTION,
            SLOT(gotoPreviousLineWithSelection()), true, tr("Go to Previous Line with Selection"));
    registerAction(GOTO_PREVIOUS_CHARACTER_WITH_SELECTION,
            SLOT(gotoPreviousCharacterWithSelection()), true, tr("Go to Previous Character with Selection"));
    registerAction(GOTO_NEXT_CHARACTER_WITH_SELECTION,
            SLOT(gotoNextCharacterWithSelection()), true, tr("Go to Next Character with Selection"));
    registerAction(GOTO_PREVIOUS_WORD_WITH_SELECTION,
            SLOT(gotoPreviousWordWithSelection()), true, tr("Go to Previous Word with Selection"));
    registerAction(GOTO_NEXT_WORD_WITH_SELECTION,
            SLOT(gotoNextWordWithSelection()), true, tr("Go to Next Word with Selection"));
    registerAction(GOTO_PREVIOUS_WORD_CAMEL_CASE_WITH_SELECTION,
            SLOT(gotoPreviousWordCamelCaseWithSelection()), false, tr("Go to Previous Word Camel Case with Selection"));
    registerAction(GOTO_NEXT_WORD_CAMEL_CASE_WITH_SELECTION,
            SLOT(gotoNextWordCamelCaseWithSelection()), false, tr("Go to Next Word Camel Case with Selection"));

    // Collect all modifying actions so we can check for them inside a readonly file
    // and disable them
    m_modifyingActions << m_pasteAction;
    m_modifyingActions << m_formatAction;
    m_modifyingActions << m_rewrapParagraphAction;
    m_modifyingActions << m_cleanWhitespaceAction;
    m_modifyingActions << m_unCommentSelectionAction;
    m_modifyingActions << m_cutLineAction;
    m_modifyingActions << m_deleteLineAction;
    m_modifyingActions << m_deleteEndOfWordAction;
    m_modifyingActions << m_deleteEndOfWordCamelCaseAction;
    m_modifyingActions << m_deleteStartOfWordAction;
    m_modifyingActions << m_deleteStartOfWordCamelCaseAction;
    m_modifyingActions << m_moveLineUpAction;
    m_modifyingActions << m_moveLineDownAction;
    m_modifyingActions << m_copyLineUpAction;
    m_modifyingActions << m_copyLineDownAction;
    m_modifyingActions << m_joinLinesAction;
    m_modifyingActions << m_insertLineAboveAction;
    m_modifyingActions << m_insertLineBelowAction;
    m_modifyingActions << m_upperCaseSelectionAction;
    m_modifyingActions << m_lowerCaseSelectionAction;
    m_modifyingActions << m_circularPasteAction;
    m_modifyingActions << m_switchUtf8bomAction;
    m_modifyingActions << m_indentAction;
    m_modifyingActions << m_unindentAction;

    // set enabled state of optional actions
    m_followSymbolAction->setEnabled(m_optionalActions & FollowSymbolUnderCursor);
    m_followSymbolInNextSplitAction->setEnabled(m_optionalActions & FollowSymbolUnderCursor);
    m_jumpToFileAction->setEnabled(m_optionalActions & JumpToFileUnderCursor);
    m_jumpToFileInNextSplitAction->setEnabled(m_optionalActions & JumpToFileUnderCursor);
    m_unfoldAllAction->setEnabled(m_optionalActions & UnCollapseAll);
}

QAction *TextEditorActionHandler::registerAction(const Core::Id &id,
                                                 const char *slot,
                                                 bool scriptable,
                                                 const QString &title,
                                                 const QKeySequence &keySequence,
                                                 const char *menueGroup,
                                                 Core::ActionContainer *container)
{
    QAction *result = new QAction(title, this);
    Core::Command *command = Core::ActionManager::registerAction(result, id, Core::Context(m_contextId), scriptable);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(keySequence);

    if (container && menueGroup)
        container->addAction(command, menueGroup);

    connect(result, SIGNAL(triggered(bool)), this, slot);
    return result;
}

void TextEditorActionHandler::updateActions()
{
    QTC_ASSERT(m_currentEditorWidget, return);
    bool isWritable = !m_currentEditorWidget->isReadOnly();
    foreach (QAction *a, m_modifyingActions)
        a->setEnabled(isWritable);
    m_formatAction->setEnabled((m_optionalActions & Format) && isWritable);
    m_unCommentSelectionAction->setEnabled((m_optionalActions & UnCommentSelection) && isWritable);
    m_visualizeWhitespaceAction->setChecked(m_currentEditorWidget->displaySettings().m_visualizeWhitespace);
    m_textWrappingAction->setChecked(m_currentEditorWidget->displaySettings().m_textWrapping);

    updateRedoAction();
    updateUndoAction();
    updateCopyAction();
}

void TextEditorActionHandler::updateRedoAction()
{
    QTC_ASSERT(m_currentEditorWidget, return);
    m_redoAction->setEnabled(m_currentEditorWidget->document()->isRedoAvailable());
}

void TextEditorActionHandler::updateUndoAction()
{
    QTC_ASSERT(m_currentEditorWidget, return);
    m_undoAction->setEnabled(m_currentEditorWidget->document()->isUndoAvailable());
}

void TextEditorActionHandler::updateCopyAction()
{
    QTC_ASSERT(m_currentEditorWidget, return);
    const bool hasCopyableText = m_currentEditorWidget->textCursor().hasSelection();
    if (m_cutAction)
        m_cutAction->setEnabled(hasCopyableText && !m_currentEditorWidget->isReadOnly());
    if (m_copyAction)
        m_copyAction->setEnabled(hasCopyableText);
}

void TextEditorActionHandler::gotoAction()
{
    QString locatorString = TextEditorPlugin::instance()->lineNumberFilter()->shortcutString();
    locatorString += QLatin1Char(' ');
    const int selectionStart = locatorString.size();
    locatorString += tr("<line>:<column>");
    Locator::LocatorManager::show(locatorString, selectionStart, locatorString.size() - selectionStart);
}

void TextEditorActionHandler::printAction()
{
    if (m_currentEditorWidget)
        m_currentEditorWidget->print(Core::ICore::printer());
}

void TextEditorActionHandler::setVisualizeWhitespace(bool checked)
{
    if (m_currentEditorWidget) {
        DisplaySettings ds = m_currentEditorWidget->displaySettings();
        ds.m_visualizeWhitespace = checked;
        m_currentEditorWidget->setDisplaySettings(ds);
    }
}

void TextEditorActionHandler::setTextWrapping(bool checked)
{
    if (m_currentEditorWidget) {
        DisplaySettings ds = m_currentEditorWidget->displaySettings();
        ds.m_textWrapping = checked;
        m_currentEditorWidget->setDisplaySettings(ds);
    }
}

#define FUNCTION(funcname) void TextEditorActionHandler::funcname ()\
{\
    if (m_currentEditorWidget)\
        m_currentEditorWidget->funcname ();\
}
#define FUNCTION2(funcname, funcname2) void TextEditorActionHandler::funcname ()\
{\
    if (m_currentEditorWidget)\
        m_currentEditorWidget->funcname2 ();\
}


FUNCTION2(undoAction, undo)
FUNCTION2(redoAction, redo)
FUNCTION2(copyAction, copy)
FUNCTION2(cutAction, cut)
FUNCTION2(pasteAction, paste)
FUNCTION2(circularPasteAction, circularPaste)
FUNCTION2(switchUtf8bomAction, switchUtf8bom)
FUNCTION2(formatAction, format)
FUNCTION2(rewrapParagraphAction, rewrapParagraph)
FUNCTION2(selectAllAction, selectAll)
FUNCTION(cleanWhitespace)
FUNCTION(unCommentSelection)
FUNCTION(cutLine)
FUNCTION(copyLine)
FUNCTION(deleteLine)
FUNCTION(deleteEndOfWord)
FUNCTION(deleteEndOfWordCamelCase)
FUNCTION(deleteStartOfWord)
FUNCTION(deleteStartOfWordCamelCase)
FUNCTION(unfoldAll)
FUNCTION(fold)
FUNCTION(unfold)
FUNCTION2(increaseFontSize, zoomIn)
FUNCTION2(decreaseFontSize, zoomOut)
FUNCTION2(resetFontSize, zoomReset)
FUNCTION(selectEncoding)
FUNCTION(gotoBlockStart)
FUNCTION(gotoBlockEnd)
FUNCTION(gotoBlockStartWithSelection)
FUNCTION(gotoBlockEndWithSelection)
FUNCTION(selectBlockUp)
FUNCTION(selectBlockDown)
FUNCTION(moveLineUp)
FUNCTION(moveLineDown)
FUNCTION(copyLineUp)
FUNCTION(copyLineDown)
FUNCTION(joinLines)
FUNCTION(uppercaseSelection)
FUNCTION(lowercaseSelection)
FUNCTION(insertLineAbove)
FUNCTION(insertLineBelow)
FUNCTION(indent)
FUNCTION(unindent)
FUNCTION(openLinkUnderCursor)
FUNCTION(openLinkUnderCursorInNextSplit)

FUNCTION(gotoLineStart)
FUNCTION(gotoLineStartWithSelection)
FUNCTION(gotoLineEnd)
FUNCTION(gotoLineEndWithSelection)
FUNCTION(gotoNextLine)
FUNCTION(gotoNextLineWithSelection)
FUNCTION(gotoPreviousLine)
FUNCTION(gotoPreviousLineWithSelection)
FUNCTION(gotoPreviousCharacter)
FUNCTION(gotoPreviousCharacterWithSelection)
FUNCTION(gotoNextCharacter)
FUNCTION(gotoNextCharacterWithSelection)
FUNCTION(gotoPreviousWord)
FUNCTION(gotoPreviousWordWithSelection)
FUNCTION(gotoPreviousWordCamelCase)
FUNCTION(gotoPreviousWordCamelCaseWithSelection)
FUNCTION(gotoNextWord)
FUNCTION(gotoNextWordWithSelection)
FUNCTION(gotoNextWordCamelCase)
FUNCTION(gotoNextWordCamelCaseWithSelection)


BaseTextEditorWidget *TextEditorActionHandler::resolveTextEditorWidget(Core::IEditor *editor) const
{
    return qobject_cast<BaseTextEditorWidget *>(editor->widget());
}

void TextEditorActionHandler::updateCurrentEditor(Core::IEditor *editor)
{
    if (m_currentEditorWidget)
        m_currentEditorWidget->disconnect(this);
    m_currentEditorWidget = 0;

    // don't need to do anything if the editor's context doesn't match
    // (our actions will be disabled because our context will not be active)
    if (!editor || !editor->context().contains(m_contextId))
        return;

    BaseTextEditorWidget *editorWidget = resolveTextEditorWidget(editor);
    QTC_ASSERT(editorWidget, return); // editor has our context id, so shouldn't happen
    m_currentEditorWidget = editorWidget;
    connect(m_currentEditorWidget, SIGNAL(undoAvailable(bool)), this, SLOT(updateUndoAction()));
    connect(m_currentEditorWidget, SIGNAL(redoAvailable(bool)), this, SLOT(updateRedoAction()));
    connect(m_currentEditorWidget, SIGNAL(copyAvailable(bool)), this, SLOT(updateCopyAction()));
    connect(m_currentEditorWidget, SIGNAL(readOnlyChanged()), this, SLOT(updateActions()));
    updateActions();
}
