/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "texteditoractionhandler.h"

#include "texteditor.h"
#include "displaysettings.h"
#include "linenumberfilter.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"

#include <coreplugin/locator/locatormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QAction>

#include <functional>

namespace TextEditor {
namespace Internal {

class TextEditorActionHandlerPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::Internal::TextEditorActionHandler)
public:
    TextEditorActionHandlerPrivate(TextEditorActionHandler *parent,
                                   Core::Id editorId,
                                   Core::Id contextId,
                                   uint optionalActions);

    QAction *registerActionHelper(Core::Id id, bool scriptable, const QString &title,
                            const QKeySequence &keySequence, Core::Id menueGroup,
                            Core::ActionContainer *container,
                            std::function<void(bool)> slot)
    {
        QAction *result = new QAction(title, this);
        Core::Command *command = Core::ActionManager::registerAction(result, id, Core::Context(m_contextId), scriptable);
        if (!keySequence.isEmpty())
            command->setDefaultKeySequence(keySequence);

        if (container && menueGroup.isValid())
            container->addAction(command, menueGroup);

        connect(result, &QAction::triggered, slot);
        return result;
    }

    QAction *registerAction(Core::Id id,
                            std::function<void(TextEditorWidget *)> slot,
                            bool scriptable = false,
                            const QString &title = QString(),
                            const QKeySequence &keySequence = QKeySequence(),
                            Core::Id menueGroup = Core::Id(),
                            Core::ActionContainer *container = 0)
    {
        return registerActionHelper(id, scriptable, title, keySequence, menueGroup, container,
            [this, slot](bool) { if (m_currentEditorWidget) slot(m_currentEditorWidget); });
    }

    QAction *registerBoolAction(Core::Id id,
                            std::function<void(TextEditorWidget *, bool)> slot,
                            bool scriptable = false,
                            const QString &title = QString(),
                            const QKeySequence &keySequence = QKeySequence(),
                            Core::Id menueGroup = Core::Id(),
                            Core::ActionContainer *container = 0)
    {
        return registerActionHelper(id, scriptable, title, keySequence, menueGroup, container,
            [this, slot](bool on) { if (m_currentEditorWidget) slot(m_currentEditorWidget, on); });
    }

    QAction *registerIntAction(Core::Id id,
                            std::function<void(TextEditorWidget *, int)> slot,
                            bool scriptable = false,
                            const QString &title = QString(),
                            const QKeySequence &keySequence = QKeySequence(),
                            Core::Id menueGroup = Core::Id(),
                            Core::ActionContainer *container = 0)
    {
        return registerActionHelper(id, scriptable, title, keySequence, menueGroup, container,
            [this, slot](bool on) { if (m_currentEditorWidget) slot(m_currentEditorWidget, on); });
    }

    void createActions();

    void updateActions();
    void updateRedoAction(bool on);
    void updateUndoAction(bool on);
    void updateCopyAction(bool on);

    void updateCurrentEditor(Core::IEditor *editor);

public:
    TextEditorActionHandler *q = nullptr;
    TextEditorActionHandler::TextEditorWidgetResolver m_findTextWidget;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_cutAction = nullptr;
    QAction *m_pasteAction = nullptr;
    QAction *m_circularPasteAction = nullptr;
    QAction *m_switchUtf8bomAction = nullptr;
    QAction *m_selectAllAction = nullptr;
    QAction *m_gotoAction = nullptr;
    QAction *m_printAction = nullptr;
    QAction *m_formatAction = nullptr;
    QAction *m_rewrapParagraphAction = nullptr;
    QAction *m_visualizeWhitespaceAction = nullptr;
    QAction *m_cleanWhitespaceAction = nullptr;
    QAction *m_textWrappingAction = nullptr;
    QAction *m_unCommentSelectionAction = nullptr;
    QAction *m_unfoldAllAction = nullptr;
    QAction *m_foldAction = nullptr;
    QAction *m_unfoldAction = nullptr;
    QAction *m_cutLineAction = nullptr;
    QAction *m_copyLineAction = nullptr;
    QAction *m_duplicateSelectionAction = nullptr;
    QAction *m_duplicateSelectionAndCommentAction = nullptr;
    QAction *m_deleteLineAction = nullptr;
    QAction *m_deleteEndOfLineAction = nullptr;
    QAction *m_deleteEndOfWordAction = nullptr;
    QAction *m_deleteEndOfWordCamelCaseAction = nullptr;
    QAction *m_deleteStartOfLineAction = nullptr;
    QAction *m_deleteStartOfWordAction = nullptr;
    QAction *m_deleteStartOfWordCamelCaseAction = nullptr;
    QAction *m_selectEncodingAction = nullptr;
    QAction *m_increaseFontSizeAction = nullptr;
    QAction *m_decreaseFontSizeAction = nullptr;
    QAction *m_resetFontSizeAction = nullptr;
    QAction *m_gotoBlockStartAction = nullptr;
    QAction *m_gotoBlockEndAction = nullptr;
    QAction *m_gotoBlockStartWithSelectionAction = nullptr;
    QAction *m_gotoBlockEndWithSelectionAction = nullptr;
    QAction *m_selectBlockUpAction = nullptr;
    QAction *m_selectBlockDownAction = nullptr;
    QAction *m_viewPageUpAction = nullptr;
    QAction *m_viewPageDownAction = nullptr;
    QAction *m_viewLineUpAction = nullptr;
    QAction *m_viewLineDownAction = nullptr;
    QAction *m_moveLineUpAction = nullptr;
    QAction *m_moveLineDownAction = nullptr;
    QAction *m_copyLineUpAction = nullptr;
    QAction *m_copyLineDownAction = nullptr;
    QAction *m_joinLinesAction = nullptr;
    QAction *m_insertLineAboveAction = nullptr;
    QAction *m_insertLineBelowAction = nullptr;
    QAction *m_upperCaseSelectionAction = nullptr;
    QAction *m_lowerCaseSelectionAction = nullptr;
    QAction *m_sortSelectedLinesAction = nullptr;
    QAction *m_indentAction = nullptr;
    QAction *m_unindentAction = nullptr;
    QAction *m_followSymbolAction = nullptr;
    QAction *m_followSymbolInNextSplitAction = nullptr;
    QAction *m_jumpToFileAction = nullptr;
    QAction *m_jumpToFileInNextSplitAction = nullptr;
    QList<QAction *> m_modifyingActions;

    uint m_optionalActions = TextEditorActionHandler::None;
    QPointer<TextEditorWidget> m_currentEditorWidget;
    Core::Id m_editorId;
    Core::Id m_contextId;
};

static TextEditorWidget *castWidgetToTextEditorWidget(Core::IEditor *editor)
{
    return qobject_cast<TextEditorWidget *>(editor->widget());
}

TextEditorActionHandlerPrivate::TextEditorActionHandlerPrivate
    (TextEditorActionHandler *parent, Core::Id editorId, Core::Id contextId, uint optionalActions)
  : q(parent)
  , m_findTextWidget(castWidgetToTextEditorWidget)
  , m_optionalActions(optionalActions)
  , m_editorId(editorId)
  , m_contextId(contextId)
{
    createActions();
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
        this, &TextEditorActionHandlerPrivate::updateCurrentEditor);
}

void TextEditorActionHandlerPrivate::createActions()
{
    using namespace Core::Constants;
    using namespace TextEditor::Constants;

    m_undoAction = registerAction(UNDO,
            [] (TextEditorWidget *w) { w->undo(); }, true, tr("&Undo"));
    m_redoAction = registerAction(REDO,
            [] (TextEditorWidget *w) { w->redo(); }, true, tr("&Redo"));
    m_copyAction = registerAction(COPY,
            [] (TextEditorWidget *w) { w->copy(); }, true);
    m_cutAction = registerAction(CUT,
            [] (TextEditorWidget *w) { w->cut(); }, true);
    m_pasteAction = registerAction(PASTE,
            [] (TextEditorWidget *w) { w->paste(); }, true);
    m_selectAllAction = registerAction(SELECTALL,
            [] (TextEditorWidget *w) { w->selectAll(); }, true);
    m_gotoAction = registerAction(GOTO, [] (TextEditorWidget *) {
            QString locatorString = TextEditorPlugin::lineNumberFilter()->shortcutString();
            locatorString += QLatin1Char(' ');
            const int selectionStart = locatorString.size();
            locatorString += TextEditorActionHandler::tr("<line>:<column>");
            Core::LocatorManager::show(locatorString, selectionStart, locatorString.size() - selectionStart);
        });
    m_printAction = registerAction(PRINT,
            [] (TextEditorWidget *widget) { widget->print(Core::ICore::printer()); });
    m_deleteLineAction = registerAction(DELETE_LINE,
            [] (TextEditorWidget *w) { w->deleteLine(); }, true, tr("Delete &Line"));
    m_deleteEndOfLineAction = registerAction(DELETE_END_OF_LINE,
            [] (TextEditorWidget *w) { w->deleteEndOfLine(); }, true, tr("Delete Line from Cursor On"));
    m_deleteEndOfWordAction = registerAction(DELETE_END_OF_WORD,
            [] (TextEditorWidget *w) { w->deleteEndOfWord(); }, true, tr("Delete Word from Cursor On"));
    m_deleteEndOfWordCamelCaseAction = registerAction(DELETE_END_OF_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->deleteEndOfWordCamelCase(); }, true, tr("Delete Word Camel Case from Cursor On"));
    m_deleteStartOfLineAction = registerAction(DELETE_START_OF_LINE,
            [] (TextEditorWidget *w) { w->deleteStartOfLine(); }, true, tr("Delete Line up to Cursor"));
    m_deleteStartOfWordAction = registerAction(DELETE_START_OF_WORD,
            [] (TextEditorWidget *w) { w->deleteStartOfWord(); }, true, tr("Delete Word up to Cursor"));
    m_deleteStartOfWordCamelCaseAction = registerAction(DELETE_START_OF_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->deleteStartOfWordCamelCase(); }, true, tr("Delete Word Camel Case up to Cursor"));
    m_gotoBlockStartWithSelectionAction = registerAction(GOTO_BLOCK_START_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoBlockStartWithSelection(); }, true, tr("Go to Block Start with Selection"),
            QKeySequence(tr("Ctrl+{")));
    m_gotoBlockEndWithSelectionAction = registerAction(GOTO_BLOCK_END_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoBlockEndWithSelection(); }, true, tr("Go to Block End with Selection"),
            QKeySequence(tr("Ctrl+}")));
    m_moveLineUpAction = registerAction(MOVE_LINE_UP,
            [] (TextEditorWidget *w) { w->moveLineUp(); }, true, tr("Move Line Up"),
            QKeySequence(tr("Ctrl+Shift+Up")));
    m_moveLineDownAction = registerAction(MOVE_LINE_DOWN,
            [] (TextEditorWidget *w) { w->moveLineDown(); }, true, tr("Move Line Down"),
            QKeySequence(tr("Ctrl+Shift+Down")));
    m_copyLineUpAction = registerAction(COPY_LINE_UP,
            [] (TextEditorWidget *w) { w->copyLineUp(); }, true, tr("Copy Line Up"),
            QKeySequence(tr("Ctrl+Alt+Up")));
    m_copyLineDownAction = registerAction(COPY_LINE_DOWN,
            [] (TextEditorWidget *w) { w->copyLineDown(); }, true, tr("Copy Line Down"),
            QKeySequence(tr("Ctrl+Alt+Down")));
    m_joinLinesAction = registerAction(JOIN_LINES,
            [] (TextEditorWidget *w) { w->joinLines(); }, true, tr("Join Lines"),
            QKeySequence(tr("Ctrl+J")));
    m_insertLineAboveAction = registerAction(INSERT_LINE_ABOVE,
            [] (TextEditorWidget *w) { w->insertLineAbove(); }, true, tr("Insert Line Above Current Line"),
            QKeySequence(tr("Ctrl+Shift+Return")));
    m_insertLineBelowAction = registerAction(INSERT_LINE_BELOW,
            [] (TextEditorWidget *w) { w->insertLineBelow(); }, true, tr("Insert Line Below Current Line"),
            QKeySequence(tr("Ctrl+Return")));
    m_switchUtf8bomAction = registerAction(SWITCH_UTF8BOM,
            [] (TextEditorWidget *w) { w->switchUtf8bom(); }, true, tr("Toggle UTF-8 BOM"));
    m_indentAction = registerAction(INDENT,
            [] (TextEditorWidget *w) { w->indent(); }, true, tr("Indent"));
    m_unindentAction = registerAction(UNINDENT,
            [] (TextEditorWidget *w) { w->unindent(); }, true, tr("Unindent"));
    m_followSymbolAction = registerAction(FOLLOW_SYMBOL_UNDER_CURSOR,
            [] (TextEditorWidget *w) { w->openLinkUnderCursor(); }, true, tr("Follow Symbol Under Cursor"),
            QKeySequence(Qt::Key_F2));
    m_followSymbolInNextSplitAction = registerAction(FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT,
            [] (TextEditorWidget *w) { w->openLinkUnderCursorInNextSplit(); }, true, tr("Follow Symbol Under Cursor in Next Split"),
            QKeySequence(Utils::HostOsInfo::isMacHost() ? tr("Meta+E, F2") : tr("Ctrl+E, F2")));
    m_jumpToFileAction = registerAction(JUMP_TO_FILE_UNDER_CURSOR,
            [] (TextEditorWidget *w) { w->openLinkUnderCursor(); }, true, tr("Jump to File Under Cursor"),
            QKeySequence(Qt::Key_F2));
    m_jumpToFileInNextSplitAction = registerAction(JUMP_TO_FILE_UNDER_CURSOR_IN_NEXT_SPLIT,
            [] (TextEditorWidget *w) { w->openLinkUnderCursorInNextSplit(); }, true, tr("Jump to File Under Cursor in Next Split"),
            QKeySequence(Utils::HostOsInfo::isMacHost() ? tr("Meta+E, F2") : tr("Ctrl+E, F2")).toString());

    m_viewPageUpAction = registerAction(VIEW_PAGE_UP,
            [] (TextEditorWidget *w) { w->viewPageUp(); }, true, tr("Move the View a Page Up and Keep the Cursor Position"),
            QKeySequence(tr("Ctrl+PgUp")));
    m_viewPageDownAction = registerAction(VIEW_PAGE_DOWN,
            [] (TextEditorWidget *w) { w->viewPageDown(); }, true, tr("Move the View a Page Down and Keep the Cursor Position"),
            QKeySequence(tr("Ctrl+PgDown")));
    m_viewLineUpAction = registerAction(VIEW_LINE_UP,
            [] (TextEditorWidget *w) { w->viewLineUp(); }, true, tr("Move the View a Line Up and Keep the Cursor Position"),
            QKeySequence(tr("Ctrl+Up")));
    m_viewLineDownAction = registerAction(VIEW_LINE_DOWN,
            [] (TextEditorWidget *w) { w->viewLineDown(); }, true, tr("Move the View a Line Down and Keep the Cursor Position"),
            QKeySequence(tr("Ctrl+Down")));

    // register "Edit" Menu Actions
    Core::ActionContainer *editMenu = Core::ActionManager::actionContainer(M_EDIT);
    m_selectEncodingAction = registerAction(SELECT_ENCODING,
            [] (TextEditorWidget *w) { w->selectEncoding(); }, false, tr("Select Encoding..."),
            QKeySequence(), G_EDIT_OTHER, editMenu);
    m_circularPasteAction = registerAction(CIRCULAR_PASTE,
            [] (TextEditorWidget *w) { w->circularPaste(); }, false, tr("Paste from Clipboard History"),
            QKeySequence(tr("Ctrl+Shift+V")), G_EDIT_COPYPASTE, editMenu);

    // register "Edit -> Advanced" Menu Actions
    Core::ActionContainer *advancedEditMenu = Core::ActionManager::actionContainer(M_EDIT_ADVANCED);
    m_formatAction = registerAction(AUTO_INDENT_SELECTION,
            [] (TextEditorWidget *w) { w->format(); }, true, tr("Auto-&indent Selection"),
            QKeySequence(tr("Ctrl+I")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_rewrapParagraphAction = registerAction(REWRAP_PARAGRAPH,
            [] (TextEditorWidget *w) { w->rewrapParagraph(); }, true, tr("&Rewrap Paragraph"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+E, R") : tr("Ctrl+E, R")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_visualizeWhitespaceAction = registerBoolAction(VISUALIZE_WHITESPACE,
            [] (TextEditorWidget *widget, bool checked) {
                if (widget) {
                     DisplaySettings ds = widget->displaySettings();
                     ds.m_visualizeWhitespace = checked;
                     widget->setDisplaySettings(ds);
                }
            },
            false, tr("&Visualize Whitespace"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+E, Meta+V") : tr("Ctrl+E, Ctrl+V")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_visualizeWhitespaceAction->setCheckable(true);
    m_cleanWhitespaceAction = registerAction(CLEAN_WHITESPACE,
            [] (TextEditorWidget *w) { w->cleanWhitespace(); }, true, tr("Clean Whitespace"),
            QKeySequence(),
            G_EDIT_FORMAT, advancedEditMenu);
    m_textWrappingAction = registerBoolAction(TEXT_WRAPPING,
            [] (TextEditorWidget *widget, bool checked) {
                if (widget) {
                    DisplaySettings ds = widget->displaySettings();
                    ds.m_textWrapping = checked;
                    widget->setDisplaySettings(ds);
                }
            },
            false, tr("Enable Text &Wrapping"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+E, Meta+W") : tr("Ctrl+E, Ctrl+W")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_textWrappingAction->setCheckable(true);
    m_unCommentSelectionAction = registerAction(UN_COMMENT_SELECTION,
            [] (TextEditorWidget *w) { w->unCommentSelection(); }, true, tr("Toggle Comment &Selection"),
            QKeySequence(tr("Ctrl+/")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_cutLineAction = registerAction(CUT_LINE,
            [] (TextEditorWidget *w) { w->cutLine(); }, true, tr("Cut &Line"),
            QKeySequence(tr("Shift+Del")),
            G_EDIT_TEXT, advancedEditMenu);
    m_copyLineAction = registerAction(COPY_LINE,
            [] (TextEditorWidget *w) { w->copyLine(); }, false, tr("Copy &Line"),
            QKeySequence(tr("Ctrl+Ins")),
            G_EDIT_TEXT, advancedEditMenu);
    m_duplicateSelectionAction = registerAction(DUPLICATE_SELECTION,
            [] (TextEditorWidget *w) { w->duplicateSelection(); }, false, tr("&Duplicate Selection"),
            QKeySequence(),
            G_EDIT_TEXT, advancedEditMenu);
    m_duplicateSelectionAndCommentAction = registerAction(DUPLICATE_SELECTION_AND_COMMENT,
            [] (TextEditorWidget *w) { w->duplicateSelectionAndComment(); }, false, tr("&Duplicate Selection and Comment"),
            QKeySequence(),
            G_EDIT_TEXT, advancedEditMenu);
    m_upperCaseSelectionAction = registerAction(UPPERCASE_SELECTION,
            [] (TextEditorWidget *w) { w->uppercaseSelection(); }, true, tr("Uppercase Selection"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+Shift+U") : tr("Alt+Shift+U")),
            G_EDIT_TEXT, advancedEditMenu);
    m_lowerCaseSelectionAction = registerAction(LOWERCASE_SELECTION,
            [] (TextEditorWidget *w) { w->lowercaseSelection(); }, true, tr("Lowercase Selection"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+U") : tr("Alt+U")),
            G_EDIT_TEXT, advancedEditMenu);
    m_sortSelectedLinesAction = registerAction(SORT_SELECTED_LINES,
            [] (TextEditorWidget *w) { w->sortSelectedLines(); }, false, tr("&Sort Selected Lines"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+Shift+S") : tr("Alt+Shift+S")),
            G_EDIT_TEXT, advancedEditMenu);
    m_foldAction = registerAction(FOLD,
            [] (TextEditorWidget *w) { w->fold(); }, true, tr("Fold"),
            QKeySequence(tr("Ctrl+<")),
            G_EDIT_COLLAPSING, advancedEditMenu);
    m_unfoldAction = registerAction(UNFOLD,
            [] (TextEditorWidget *w) { w->unfold(); }, true, tr("Unfold"),
            QKeySequence(tr("Ctrl+>")),
            G_EDIT_COLLAPSING, advancedEditMenu);
    m_unfoldAllAction = registerAction(UNFOLD_ALL,
            [] (TextEditorWidget *w) { w->unfoldAll(); }, true, tr("Toggle &Fold All"),
            QKeySequence(),
            G_EDIT_COLLAPSING, advancedEditMenu);
    m_increaseFontSizeAction = registerAction(INCREASE_FONT_SIZE,
            [] (TextEditorWidget *w) { w->zoomF(1.f); }, false, tr("Increase Font Size"),
            QKeySequence(tr("Ctrl++")),
            G_EDIT_FONT, advancedEditMenu);
    m_decreaseFontSizeAction = registerAction(DECREASE_FONT_SIZE,
            [] (TextEditorWidget *w) { w->zoomF(-1.f); }, false, tr("Decrease Font Size"),
            QKeySequence(tr("Ctrl+-")),
            G_EDIT_FONT, advancedEditMenu);
    m_resetFontSizeAction = registerAction(RESET_FONT_SIZE,
            [] (TextEditorWidget *w) { w->zoomReset(); }, false, tr("Reset Font Size"),
            QKeySequence(Core::UseMacShortcuts ? tr("Meta+0") : tr("Ctrl+0")),
            G_EDIT_FONT, advancedEditMenu);
    m_gotoBlockStartAction = registerAction(GOTO_BLOCK_START,
            [] (TextEditorWidget *w) { w->gotoBlockStart(); }, true, tr("Go to Block Start"),
            QKeySequence(tr("Ctrl+[")),
            G_EDIT_BLOCKS, advancedEditMenu);
    m_gotoBlockEndAction = registerAction(GOTO_BLOCK_END,
            [] (TextEditorWidget *w) { w->gotoBlockEnd(); }, true, tr("Go to Block End"),
            QKeySequence(tr("Ctrl+]")),
            G_EDIT_BLOCKS, advancedEditMenu);
    m_selectBlockUpAction = registerAction(SELECT_BLOCK_UP,
            [] (TextEditorWidget *w) { w->selectBlockUp(); }, true, tr("Select Block Up"),
            QKeySequence(tr("Ctrl+U")),
            G_EDIT_BLOCKS, advancedEditMenu);
    m_selectBlockDownAction = registerAction(SELECT_BLOCK_DOWN,
            [] (TextEditorWidget *w) { w->selectBlockDown(); }, true, tr("Select Block Down"),
            QKeySequence(tr("Ctrl+Shift+Alt+U")),
            G_EDIT_BLOCKS, advancedEditMenu);
    registerAction(SELECT_WORD_UNDER_CURSOR,
            [] (TextEditorWidget *w) { w->selectWordUnderCursor(); }, true,
            tr("Select Word Under Cursor"));

    // register GOTO Actions
    registerAction(GOTO_LINE_START,
            [] (TextEditorWidget *w) { w->gotoLineStart(); }, true, tr("Go to Line Start"));
    registerAction(GOTO_LINE_END,
            [] (TextEditorWidget *w) { w->gotoLineEnd(); }, true, tr("Go to Line End"));
    registerAction(GOTO_NEXT_LINE,
            [] (TextEditorWidget *w) { w->gotoNextLine(); }, true, tr("Go to Next Line"));
    registerAction(GOTO_PREVIOUS_LINE,
            [] (TextEditorWidget *w) { w->gotoPreviousLine(); }, true, tr("Go to Previous Line"));
    registerAction(GOTO_PREVIOUS_CHARACTER,
            [] (TextEditorWidget *w) { w->gotoPreviousCharacter(); }, true, tr("Go to Previous Character"));
    registerAction(GOTO_NEXT_CHARACTER,
            [] (TextEditorWidget *w) { w->gotoNextCharacter(); }, true, tr("Go to Next Character"));
    registerAction(GOTO_PREVIOUS_WORD,
            [] (TextEditorWidget *w) { w->gotoPreviousWord(); }, true, tr("Go to Previous Word"));
    registerAction(GOTO_NEXT_WORD,
            [] (TextEditorWidget *w) { w->gotoNextWord(); }, true, tr("Go to Next Word"));
    registerAction(GOTO_PREVIOUS_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->gotoPreviousWordCamelCase(); }, false, tr("Go to Previous Word Camel Case"));
    registerAction(GOTO_NEXT_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->gotoNextWordCamelCase(); }, false, tr("Go to Next Word Camel Case"));

    // register GOTO actions with selection
    registerAction(GOTO_LINE_START_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoLineStartWithSelection(); }, true, tr("Go to Line Start with Selection"));
    registerAction(GOTO_LINE_END_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoLineEndWithSelection(); }, true, tr("Go to Line End with Selection"));
    registerAction(GOTO_NEXT_LINE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextLineWithSelection(); }, true, tr("Go to Next Line with Selection"));
    registerAction(GOTO_PREVIOUS_LINE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousLineWithSelection(); }, true, tr("Go to Previous Line with Selection"));
    registerAction(GOTO_PREVIOUS_CHARACTER_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousCharacterWithSelection(); }, true, tr("Go to Previous Character with Selection"));
    registerAction(GOTO_NEXT_CHARACTER_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextCharacterWithSelection(); }, true, tr("Go to Next Character with Selection"));
    registerAction(GOTO_PREVIOUS_WORD_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousWordWithSelection(); }, true, tr("Go to Previous Word with Selection"));
    registerAction(GOTO_NEXT_WORD_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextWordWithSelection(); }, true, tr("Go to Next Word with Selection"));
    registerAction(GOTO_PREVIOUS_WORD_CAMEL_CASE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousWordCamelCaseWithSelection(); }, false, tr("Go to Previous Word Camel Case with Selection"));
    registerAction(GOTO_NEXT_WORD_CAMEL_CASE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextWordCamelCaseWithSelection(); }, false, tr("Go to Next Word Camel Case with Selection"));

    // Collect all modifying actions so we can check for them inside a readonly file
    // and disable them
    m_modifyingActions << m_circularPasteAction;
    m_modifyingActions << m_cleanWhitespaceAction;
    m_modifyingActions << m_copyLineDownAction;
    m_modifyingActions << m_copyLineUpAction;
    m_modifyingActions << m_cutLineAction;
    m_modifyingActions << m_deleteEndOfLineAction;
    m_modifyingActions << m_deleteEndOfWordAction;
    m_modifyingActions << m_deleteEndOfWordCamelCaseAction;
    m_modifyingActions << m_deleteLineAction;
    m_modifyingActions << m_deleteStartOfLineAction;
    m_modifyingActions << m_deleteStartOfWordAction;
    m_modifyingActions << m_deleteStartOfWordCamelCaseAction;
    m_modifyingActions << m_duplicateSelectionAction;
    m_modifyingActions << m_duplicateSelectionAndCommentAction;
    m_modifyingActions << m_formatAction;
    m_modifyingActions << m_indentAction;
    m_modifyingActions << m_insertLineAboveAction;
    m_modifyingActions << m_insertLineBelowAction;
    m_modifyingActions << m_joinLinesAction;
    m_modifyingActions << m_lowerCaseSelectionAction;
    m_modifyingActions << m_moveLineDownAction;
    m_modifyingActions << m_moveLineUpAction;
    m_modifyingActions << m_pasteAction;
    m_modifyingActions << m_rewrapParagraphAction;
    m_modifyingActions << m_switchUtf8bomAction;
    m_modifyingActions << m_unCommentSelectionAction;
    m_modifyingActions << m_unindentAction;
    m_modifyingActions << m_upperCaseSelectionAction;
    m_modifyingActions << m_sortSelectedLinesAction;

    // set enabled state of optional actions
    m_followSymbolAction->setEnabled(m_optionalActions & TextEditorActionHandler::FollowSymbolUnderCursor);
    m_followSymbolInNextSplitAction->setEnabled(m_optionalActions & TextEditorActionHandler::FollowSymbolUnderCursor);
    m_jumpToFileAction->setEnabled(m_optionalActions & TextEditorActionHandler::JumpToFileUnderCursor);
    m_jumpToFileInNextSplitAction->setEnabled(m_optionalActions & TextEditorActionHandler::JumpToFileUnderCursor);
    m_unfoldAllAction->setEnabled(m_optionalActions & TextEditorActionHandler::UnCollapseAll);
}

void TextEditorActionHandlerPrivate::updateActions()
{
    bool isWritable = m_currentEditorWidget && !m_currentEditorWidget->isReadOnly();
    foreach (QAction *a, m_modifyingActions)
        a->setEnabled(isWritable);
    m_formatAction->setEnabled((m_optionalActions & TextEditorActionHandler::Format) && isWritable);
    m_unCommentSelectionAction->setEnabled((m_optionalActions & TextEditorActionHandler::UnCommentSelection) && isWritable);
    m_visualizeWhitespaceAction->setEnabled(m_currentEditorWidget);
    m_textWrappingAction->setEnabled(m_currentEditorWidget);
    if (m_currentEditorWidget) {
        m_visualizeWhitespaceAction->setChecked(
                    m_currentEditorWidget->displaySettings().m_visualizeWhitespace);
        m_textWrappingAction->setChecked(m_currentEditorWidget->displaySettings().m_textWrapping);
    }

    updateRedoAction(m_currentEditorWidget && m_currentEditorWidget->document()->isRedoAvailable());
    updateUndoAction(m_currentEditorWidget && m_currentEditorWidget->document()->isUndoAvailable());
    updateCopyAction(m_currentEditorWidget && m_currentEditorWidget->textCursor().hasSelection());
}

void TextEditorActionHandlerPrivate::updateRedoAction(bool on)
{
    m_redoAction->setEnabled(on);
}

void TextEditorActionHandlerPrivate::updateUndoAction(bool on)
{
    m_undoAction->setEnabled(on);
}

void TextEditorActionHandlerPrivate::updateCopyAction(bool hasCopyableText)
{
    if (m_cutAction)
        m_cutAction->setEnabled(hasCopyableText && m_currentEditorWidget
                                && !m_currentEditorWidget->isReadOnly());
    if (m_copyAction)
        m_copyAction->setEnabled(hasCopyableText);
}

void TextEditorActionHandlerPrivate::updateCurrentEditor(Core::IEditor *editor)
{
    if (m_currentEditorWidget)
        m_currentEditorWidget->disconnect(this);
    m_currentEditorWidget = 0;

    if (editor && editor->document()->id() == m_editorId) {
        TextEditorWidget *editorWidget = m_findTextWidget(editor);
        QTC_ASSERT(editorWidget, return); // editor has our id, so shouldn't happen
        m_currentEditorWidget = editorWidget;
        connect(editorWidget, &QPlainTextEdit::undoAvailable,
                this, &TextEditorActionHandlerPrivate::updateUndoAction);
        connect(editorWidget, &QPlainTextEdit::redoAvailable,
                this, &TextEditorActionHandlerPrivate::updateRedoAction);
        connect(editorWidget, &QPlainTextEdit::copyAvailable,
                this, &TextEditorActionHandlerPrivate::updateCopyAction);
        connect(editorWidget, &TextEditorWidget::readOnlyChanged,
                this, &TextEditorActionHandlerPrivate::updateActions);
    }
    updateActions();
}

} // namespace Internal

TextEditorActionHandler::TextEditorActionHandler(QObject *parent, Core::Id editorId,
                                                 Core::Id contextId, uint optionalActions)
    : QObject(parent), d(new Internal::TextEditorActionHandlerPrivate(this, editorId, contextId,
                                                                      optionalActions))
{
}

TextEditorActionHandler::~TextEditorActionHandler()
{
    delete d;
}

void TextEditorActionHandler::setTextEditorWidgetResolver(const TextEditorWidgetResolver &resolver)
{
    d->m_findTextWidget = resolver;
}

} // namespace TextEditor
