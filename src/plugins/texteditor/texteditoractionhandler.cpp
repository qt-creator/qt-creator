// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditoractionhandler.h"

#include "texteditor.h"
#include "displaysettings.h"
#include "fontsettings.h"
#include "linenumberfilter.h"
#include "texteditorplugin.h"
#include "texteditortr.h"
#include "texteditorsettings.h"

#include <aggregation/aggregate.h>

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
public:
    TextEditorActionHandlerPrivate(Utils::Id editorId, Utils::Id contextId, uint optionalActions);

    QAction *registerActionHelper(Utils::Id id, bool scriptable, const QString &title,
                            const QKeySequence &keySequence, Utils::Id menueGroup,
                            Core::ActionContainer *container,
                            std::function<void(bool)> slot)
    {
        auto result = new QAction(title, this);
        Core::Command *command = Core::ActionManager::registerAction(result, id, Core::Context(m_contextId), scriptable);
        if (!keySequence.isEmpty())
            command->setDefaultKeySequence(keySequence);

        if (container && menueGroup.isValid())
            container->addAction(command, menueGroup);

        connect(result, &QAction::triggered, slot);
        return result;
    }

    QAction *registerAction(Utils::Id id,
                            std::function<void(TextEditorWidget *)> slot,
                            bool scriptable = false,
                            const QString &title = QString(),
                            const QKeySequence &keySequence = QKeySequence(),
                            Utils::Id menueGroup = Utils::Id(),
                            Core::ActionContainer *container = nullptr)
    {
        return registerActionHelper(id,
                                    scriptable,
                                    title,
                                    keySequence,
                                    menueGroup,
                                    container,
                                    [this, slot, id](bool) {
                                        if (m_currentEditorWidget)
                                            slot(m_currentEditorWidget);
                                        else if (m_unhandledCallback)
                                            m_unhandledCallback(id, m_currentEditor);
                                    });
    }

    QAction *registerBoolAction(Utils::Id id,
                            std::function<void(TextEditorWidget *, bool)> slot,
                            bool scriptable = false,
                            const QString &title = QString(),
                            const QKeySequence &keySequence = QKeySequence(),
                            Utils::Id menueGroup = Utils::Id(),
                            Core::ActionContainer *container = nullptr)
    {
        return registerActionHelper(id, scriptable, title, keySequence, menueGroup, container,
            [this, slot](bool on) { if (m_currentEditorWidget) slot(m_currentEditorWidget, on); });
    }

    QAction *registerIntAction(Utils::Id id,
                            std::function<void(TextEditorWidget *, int)> slot,
                            bool scriptable = false,
                            const QString &title = QString(),
                            const QKeySequence &keySequence = QKeySequence(),
                            Utils::Id menueGroup = Utils::Id(),
                            Core::ActionContainer *container = nullptr)
    {
        return registerActionHelper(id, scriptable, title, keySequence, menueGroup, container,
            [this, slot](bool on) { if (m_currentEditorWidget) slot(m_currentEditorWidget, on); });
    }

    void createActions();

    void updateActions();
    void updateOptionalActions();
    void updateRedoAction(bool on);
    void updateUndoAction(bool on);
    void updateCopyAction(bool on);

    void updateCurrentEditor(Core::IEditor *editor);

    void setCanUndoCallback(const TextEditorActionHandler::Predicate &callback);
    void setCanRedoCallback(const TextEditorActionHandler::Predicate &callback);

public:
    TextEditorActionHandler::TextEditorWidgetResolver m_findTextWidget;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_copyHtmlAction = nullptr;
    QAction *m_cutAction = nullptr;
    QAction *m_autoIndentAction = nullptr;
    QAction *m_autoFormatAction = nullptr;
    QAction *m_visualizeWhitespaceAction = nullptr;
    QAction *m_textWrappingAction = nullptr;
    QAction *m_unCommentSelectionAction = nullptr;
    QAction *m_unfoldAllAction = nullptr;
    QAction *m_followSymbolAction = nullptr;
    QAction *m_followSymbolInNextSplitAction = nullptr;
    QAction *m_followToTypeAction = nullptr;
    QAction *m_followToTypeInNextSplitAction = nullptr;
    QAction *m_findUsageAction = nullptr;
    QAction *m_openCallHierarchyAction = nullptr;
    QAction *m_renameSymbolAction = nullptr;
    QAction *m_jumpToFileAction = nullptr;
    QAction *m_jumpToFileInNextSplitAction = nullptr;
    QList<QAction *> m_modifyingActions;

    uint m_optionalActions = TextEditorActionHandler::None;
    QPointer<TextEditorWidget> m_currentEditorWidget;
    QPointer<Core::IEditor> m_currentEditor;
    Utils::Id m_editorId;
    Utils::Id m_contextId;

    TextEditorActionHandler::Predicate m_canUndoCallback;
    TextEditorActionHandler::Predicate m_canRedoCallback;

    TextEditorActionHandler::UnhandledCallback m_unhandledCallback;
};

TextEditorActionHandlerPrivate::TextEditorActionHandlerPrivate
    (Utils::Id editorId, Utils::Id contextId, uint optionalActions)
  : m_optionalActions(optionalActions)
  , m_editorId(editorId)
  , m_contextId(contextId)
{
    createActions();
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
        this, &TextEditorActionHandlerPrivate::updateCurrentEditor);
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
        this, &TextEditorActionHandlerPrivate::updateActions);
}

void TextEditorActionHandlerPrivate::createActions()
{
    using namespace Core::Constants;
    using namespace TextEditor::Constants;

    m_undoAction = registerAction(UNDO,
            [] (TextEditorWidget *w) { w->undo(); }, true, Tr::tr("&Undo"));
    m_redoAction = registerAction(REDO,
            [] (TextEditorWidget *w) { w->redo(); }, true, Tr::tr("&Redo"));
    m_copyAction = registerAction(COPY,
            [] (TextEditorWidget *w) { w->copy(); }, true);
    m_cutAction = registerAction(CUT,
            [] (TextEditorWidget *w) { w->cut(); }, true);
    m_modifyingActions << registerAction(PASTE,
            [] (TextEditorWidget *w) { w->paste(); }, true);
    registerAction(SELECTALL,
            [] (TextEditorWidget *w) { w->selectAll(); }, true);
    registerAction(GOTO, [] (TextEditorWidget *) {
            Core::LocatorManager::showFilter(TextEditorPlugin::lineNumberFilter());
        });
    m_modifyingActions << registerAction(PRINT,
            [] (TextEditorWidget *widget) { widget->print(Core::ICore::printer()); });
    m_modifyingActions << registerAction(DELETE_LINE,
            [] (TextEditorWidget *w) { w->deleteLine(); }, true, Tr::tr("Delete &Line"));
    m_modifyingActions << registerAction(DELETE_END_OF_LINE,
            [] (TextEditorWidget *w) { w->deleteEndOfLine(); }, true, Tr::tr("Delete Line from Cursor On"));
    m_modifyingActions << registerAction(DELETE_END_OF_WORD,
            [] (TextEditorWidget *w) { w->deleteEndOfWord(); }, true, Tr::tr("Delete Word from Cursor On"));
    m_modifyingActions << registerAction(DELETE_END_OF_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->deleteEndOfWordCamelCase(); }, true, Tr::tr("Delete Word Camel Case from Cursor On"));
    m_modifyingActions << registerAction(DELETE_START_OF_LINE,
            [] (TextEditorWidget *w) { w->deleteStartOfLine(); }, true, Tr::tr("Delete Line up to Cursor"),
            Core::useMacShortcuts ? QKeySequence(Tr::tr("Ctrl+Backspace")) : QKeySequence());
    m_modifyingActions << registerAction(DELETE_START_OF_WORD,
            [] (TextEditorWidget *w) { w->deleteStartOfWord(); }, true, Tr::tr("Delete Word up to Cursor"));
    m_modifyingActions << registerAction(DELETE_START_OF_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->deleteStartOfWordCamelCase(); }, true, Tr::tr("Delete Word Camel Case up to Cursor"));
    registerAction(GOTO_BLOCK_START_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoBlockStartWithSelection(); }, true, Tr::tr("Go to Block Start with Selection"),
            QKeySequence(Tr::tr("Ctrl+{")));
    registerAction(GOTO_BLOCK_END_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoBlockEndWithSelection(); }, true, Tr::tr("Go to Block End with Selection"),
            QKeySequence(Tr::tr("Ctrl+}")));
    m_modifyingActions << registerAction(MOVE_LINE_UP,
            [] (TextEditorWidget *w) { w->moveLineUp(); }, true, Tr::tr("Move Line Up"),
            QKeySequence(Tr::tr("Ctrl+Shift+Up")));
    m_modifyingActions << registerAction(MOVE_LINE_DOWN,
            [] (TextEditorWidget *w) { w->moveLineDown(); }, true, Tr::tr("Move Line Down"),
            QKeySequence(Tr::tr("Ctrl+Shift+Down")));
    m_modifyingActions << registerAction(COPY_LINE_UP,
            [] (TextEditorWidget *w) { w->copyLineUp(); }, true, Tr::tr("Copy Line Up"),
            QKeySequence(Tr::tr("Ctrl+Alt+Up")));
    m_modifyingActions << registerAction(COPY_LINE_DOWN,
            [] (TextEditorWidget *w) { w->copyLineDown(); }, true, Tr::tr("Copy Line Down"),
            QKeySequence(Tr::tr("Ctrl+Alt+Down")));
    m_modifyingActions << registerAction(JOIN_LINES,
            [] (TextEditorWidget *w) { w->joinLines(); }, true, Tr::tr("Join Lines"),
            QKeySequence(Tr::tr("Ctrl+J")));
    m_modifyingActions << registerAction(INSERT_LINE_ABOVE,
            [] (TextEditorWidget *w) { w->insertLineAbove(); }, true, Tr::tr("Insert Line Above Current Line"),
            QKeySequence(Tr::tr("Ctrl+Shift+Return")));
    m_modifyingActions << registerAction(INSERT_LINE_BELOW,
            [] (TextEditorWidget *w) { w->insertLineBelow(); }, true, Tr::tr("Insert Line Below Current Line"),
            QKeySequence(Tr::tr("Ctrl+Return")));
    m_modifyingActions << registerAction(SWITCH_UTF8BOM,
            [] (TextEditorWidget *w) { w->switchUtf8bom(); }, true, Tr::tr("Toggle UTF-8 BOM"));
    m_modifyingActions << registerAction(INDENT,
            [] (TextEditorWidget *w) { w->indent(); }, true, Tr::tr("Indent"));
    m_modifyingActions << registerAction(UNINDENT,
            [] (TextEditorWidget *w) { w->unindent(); }, true, Tr::tr("Unindent"));
    m_followSymbolAction = registerAction(FOLLOW_SYMBOL_UNDER_CURSOR,
            [] (TextEditorWidget *w) { w->openLinkUnderCursor(); }, true, Tr::tr("Follow Symbol Under Cursor"),
            QKeySequence(Qt::Key_F2));
    m_followSymbolInNextSplitAction = registerAction(FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT,
            [] (TextEditorWidget *w) { w->openLinkUnderCursorInNextSplit(); }, true, Tr::tr("Follow Symbol Under Cursor in Next Split"),
            QKeySequence(Utils::HostOsInfo::isMacHost() ? Tr::tr("Meta+E, F2") : Tr::tr("Ctrl+E, F2")));
    m_followToTypeAction = registerAction(FOLLOW_SYMBOL_TO_TYPE,
            [] (TextEditorWidget *w) { w->openTypeUnderCursor(); }, true, Tr::tr("Follow Type Under Cursor"),
            QKeySequence(Tr::tr("Ctrl+Shift+F2")));
    m_followToTypeInNextSplitAction = registerAction(FOLLOW_SYMBOL_TO_TYPE_IN_NEXT_SPLIT,
            [] (TextEditorWidget *w) { w->openTypeUnderCursorInNextSplit(); }, true, Tr::tr("Follow Type Under Cursor in Next Split"),
            QKeySequence(Utils::HostOsInfo::isMacHost() ? Tr::tr("Meta+E, Shift+F2") : Tr::tr("Ctrl+E, Ctrl+Shift+F2")));
    m_findUsageAction = registerAction(FIND_USAGES,
            [] (TextEditorWidget *w) { w->findUsages(); }, true, Tr::tr("Find References to Symbol Under Cursor"),
            QKeySequence(Tr::tr("Ctrl+Shift+U")));
    m_renameSymbolAction = registerAction(RENAME_SYMBOL,
            [] (TextEditorWidget *w) { w->renameSymbolUnderCursor(); }, true, Tr::tr("Rename Symbol Under Cursor"),
            QKeySequence(Tr::tr("Ctrl+Shift+R")));
    m_jumpToFileAction = registerAction(JUMP_TO_FILE_UNDER_CURSOR,
            [] (TextEditorWidget *w) { w->openLinkUnderCursor(); }, true, Tr::tr("Jump to File Under Cursor"),
            QKeySequence(Qt::Key_F2));
    m_jumpToFileInNextSplitAction = registerAction(JUMP_TO_FILE_UNDER_CURSOR_IN_NEXT_SPLIT,
            [] (TextEditorWidget *w) { w->openLinkUnderCursorInNextSplit(); }, true, Tr::tr("Jump to File Under Cursor in Next Split"),
            QKeySequence(Utils::HostOsInfo::isMacHost() ? Tr::tr("Meta+E, F2") : Tr::tr("Ctrl+E, F2")).toString());
    m_openCallHierarchyAction = registerAction(OPEN_CALL_HIERARCHY,
            [] (TextEditorWidget *w) { w->openCallHierarchy(); }, true, Tr::tr("Open Call Hierarchy"));

    registerAction(VIEW_PAGE_UP,
            [] (TextEditorWidget *w) { w->viewPageUp(); }, true, Tr::tr("Move the View a Page Up and Keep the Cursor Position"),
            QKeySequence(Tr::tr("Ctrl+PgUp")));
    registerAction(VIEW_PAGE_DOWN,
            [] (TextEditorWidget *w) { w->viewPageDown(); }, true, Tr::tr("Move the View a Page Down and Keep the Cursor Position"),
            QKeySequence(Tr::tr("Ctrl+PgDown")));
    registerAction(VIEW_LINE_UP,
            [] (TextEditorWidget *w) { w->viewLineUp(); }, true, Tr::tr("Move the View a Line Up and Keep the Cursor Position"),
            QKeySequence(Tr::tr("Ctrl+Up")));
    registerAction(VIEW_LINE_DOWN,
            [] (TextEditorWidget *w) { w->viewLineDown(); }, true, Tr::tr("Move the View a Line Down and Keep the Cursor Position"),
            QKeySequence(Tr::tr("Ctrl+Down")));

    // register "Edit" Menu Actions
    Core::ActionContainer *editMenu = Core::ActionManager::actionContainer(M_EDIT);
    registerAction(SELECT_ENCODING,
            [] (TextEditorWidget *w) { w->selectEncoding(); }, false, Tr::tr("Select Encoding..."),
            QKeySequence(), G_EDIT_OTHER, editMenu);
    m_modifyingActions << registerAction(CIRCULAR_PASTE,
        [] (TextEditorWidget *w) { w->circularPaste(); }, false, Tr::tr("Paste from Clipboard History"),
        QKeySequence(Tr::tr("Ctrl+Shift+V")), G_EDIT_COPYPASTE, editMenu);
    m_modifyingActions << registerAction(NO_FORMAT_PASTE,
        [] (TextEditorWidget *w) { w->pasteWithoutFormat(); }, false, Tr::tr("Paste Without Formatting"),
        QKeySequence(Core::useMacShortcuts ? Tr::tr("Ctrl+Alt+Shift+V") : QString()), G_EDIT_COPYPASTE, editMenu);

    // register "Edit -> Advanced" Menu Actions
    Core::ActionContainer *advancedEditMenu = Core::ActionManager::actionContainer(M_EDIT_ADVANCED);
    m_autoIndentAction = registerAction(AUTO_INDENT_SELECTION,
            [] (TextEditorWidget *w) { w->autoIndent(); }, true, Tr::tr("Auto-&indent Selection"),
            QKeySequence(Tr::tr("Ctrl+I")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_autoFormatAction = registerAction(AUTO_FORMAT_SELECTION,
            [] (TextEditorWidget *w) { w->autoFormat(); }, true, Tr::tr("Auto-&format Selection"),
            QKeySequence(Tr::tr("Ctrl+;")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_modifyingActions << registerAction(REWRAP_PARAGRAPH,
            [] (TextEditorWidget *w) { w->rewrapParagraph(); }, true, Tr::tr("&Rewrap Paragraph"),
            QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+E, R") : Tr::tr("Ctrl+E, R")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_visualizeWhitespaceAction = registerBoolAction(VISUALIZE_WHITESPACE,
            [] (TextEditorWidget *widget, bool checked) {
                if (widget) {
                     DisplaySettings ds = widget->displaySettings();
                     ds.m_visualizeWhitespace = checked;
                     widget->setDisplaySettings(ds);
                }
            },
            false, Tr::tr("&Visualize Whitespace"),
            QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+E, Meta+V") : Tr::tr("Ctrl+E, Ctrl+V")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_visualizeWhitespaceAction->setCheckable(true);
    m_modifyingActions << registerAction(CLEAN_WHITESPACE,
            [] (TextEditorWidget *w) { w->cleanWhitespace(); }, true, Tr::tr("Clean Whitespace"),
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
            false, Tr::tr("Enable Text &Wrapping"),
            QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+E, Meta+W") : Tr::tr("Ctrl+E, Ctrl+W")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_textWrappingAction->setCheckable(true);
    m_unCommentSelectionAction = registerAction(UN_COMMENT_SELECTION,
            [] (TextEditorWidget *w) { w->unCommentSelection(); }, true, Tr::tr("Toggle Comment &Selection"),
            QKeySequence(Tr::tr("Ctrl+/")),
            G_EDIT_FORMAT, advancedEditMenu);
    m_modifyingActions << registerAction(CUT_LINE,
            [] (TextEditorWidget *w) { w->cutLine(); }, true, Tr::tr("Cut &Line"),
            QKeySequence(Tr::tr("Shift+Del")),
            G_EDIT_TEXT, advancedEditMenu);
    registerAction(COPY_LINE,
            [] (TextEditorWidget *w) { w->copyLine(); }, false, Tr::tr("Copy &Line"),
            QKeySequence(Tr::tr("Ctrl+Ins")),
            G_EDIT_TEXT, advancedEditMenu);
    m_copyHtmlAction = registerAction(COPY_WITH_HTML,
            [] (TextEditorWidget *w) { w->copyWithHtml(); }, true, Tr::tr("Copy With Highlighting"),
            QKeySequence(), G_EDIT_TEXT, advancedEditMenu);

    registerAction(ADD_CURSORS_TO_LINE_ENDS,
            [] (TextEditorWidget *w) { w->addCursorsToLineEnds(); }, false, Tr::tr("Create Cursors at Selected Line Ends"),
            QKeySequence(Tr::tr("Alt+Shift+I")),
            G_EDIT_TEXT, advancedEditMenu);
    registerAction(ADD_SELECT_NEXT_FIND_MATCH,
            [] (TextEditorWidget *w) { w->addSelectionNextFindMatch(); }, false, Tr::tr("Add Next Occurrence to Selection"),
            QKeySequence(Tr::tr("Ctrl+D")),
            G_EDIT_TEXT, advancedEditMenu);
    m_modifyingActions << registerAction(DUPLICATE_SELECTION,
            [] (TextEditorWidget *w) { w->duplicateSelection(); }, false, Tr::tr("&Duplicate Selection"),
            QKeySequence(),
            G_EDIT_TEXT, advancedEditMenu);
    m_modifyingActions << registerAction(DUPLICATE_SELECTION_AND_COMMENT,
            [] (TextEditorWidget *w) { w->duplicateSelectionAndComment(); }, false, Tr::tr("&Duplicate Selection and Comment"),
            QKeySequence(),
            G_EDIT_TEXT, advancedEditMenu);
    m_modifyingActions << registerAction(UPPERCASE_SELECTION,
            [] (TextEditorWidget *w) { w->uppercaseSelection(); }, true, Tr::tr("Uppercase Selection"),
            QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+Shift+U") : Tr::tr("Alt+Shift+U")),
            G_EDIT_TEXT, advancedEditMenu);
    m_modifyingActions << registerAction(LOWERCASE_SELECTION,
            [] (TextEditorWidget *w) { w->lowercaseSelection(); }, true, Tr::tr("Lowercase Selection"),
            QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+U") : Tr::tr("Alt+U")),
            G_EDIT_TEXT, advancedEditMenu);
    m_modifyingActions << registerAction(SORT_SELECTED_LINES,
            [] (TextEditorWidget *w) { w->sortSelectedLines(); }, false, Tr::tr("&Sort Selected Lines"),
            QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+Shift+S") : Tr::tr("Alt+Shift+S")),
            G_EDIT_TEXT, advancedEditMenu);
    registerAction(FOLD,
            [] (TextEditorWidget *w) { w->foldCurrentBlock(); }, true, Tr::tr("Fold"),
            QKeySequence(Tr::tr("Ctrl+<")),
            G_EDIT_COLLAPSING, advancedEditMenu);
    registerAction(UNFOLD,
            [] (TextEditorWidget *w) { w->unfoldCurrentBlock(); }, true, Tr::tr("Unfold"),
            QKeySequence(Tr::tr("Ctrl+>")),
            G_EDIT_COLLAPSING, advancedEditMenu);
    m_unfoldAllAction = registerAction(UNFOLD_ALL,
            [] (TextEditorWidget *w) { w->unfoldAll(); }, true, Tr::tr("Toggle &Fold All"),
            QKeySequence(),
            G_EDIT_COLLAPSING, advancedEditMenu);
    registerAction(INCREASE_FONT_SIZE,
            [] (TextEditorWidget *w) { w->zoomF(1.f); }, false, Tr::tr("Increase Font Size"),
            QKeySequence(Tr::tr("Ctrl++")),
            G_EDIT_FONT, advancedEditMenu);
    registerAction(DECREASE_FONT_SIZE,
            [] (TextEditorWidget *w) { w->zoomF(-1.f); }, false, Tr::tr("Decrease Font Size"),
            QKeySequence(Tr::tr("Ctrl+-")),
            G_EDIT_FONT, advancedEditMenu);
    registerAction(RESET_FONT_SIZE,
            [] (TextEditorWidget *w) { w->zoomReset(); }, false, Tr::tr("Reset Font Size"),
            QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+0") : Tr::tr("Ctrl+0")),
            G_EDIT_FONT, advancedEditMenu);
    registerAction(GOTO_BLOCK_START,
            [] (TextEditorWidget *w) { w->gotoBlockStart(); }, true, Tr::tr("Go to Block Start"),
            QKeySequence(Tr::tr("Ctrl+[")),
            G_EDIT_BLOCKS, advancedEditMenu);
    registerAction(GOTO_BLOCK_END,
            [] (TextEditorWidget *w) { w->gotoBlockEnd(); }, true, Tr::tr("Go to Block End"),
            QKeySequence(Tr::tr("Ctrl+]")),
            G_EDIT_BLOCKS, advancedEditMenu);
    registerAction(SELECT_BLOCK_UP,
            [] (TextEditorWidget *w) { w->selectBlockUp(); }, true, Tr::tr("Select Block Up"),
            QKeySequence(Tr::tr("Ctrl+U")),
            G_EDIT_BLOCKS, advancedEditMenu);
    registerAction(SELECT_BLOCK_DOWN,
            [] (TextEditorWidget *w) { w->selectBlockDown(); }, true, Tr::tr("Select Block Down"),
            QKeySequence(Tr::tr("Ctrl+Shift+Alt+U")),
            G_EDIT_BLOCKS, advancedEditMenu);
    registerAction(SELECT_WORD_UNDER_CURSOR,
            [] (TextEditorWidget *w) { w->selectWordUnderCursor(); }, true,
            Tr::tr("Select Word Under Cursor"));

    // register GOTO Actions
    registerAction(GOTO_DOCUMENT_START,
            [] (TextEditorWidget *w) { w->gotoDocumentStart(); }, true, Tr::tr("Go to Document Start"));
    registerAction(GOTO_DOCUMENT_END,
            [] (TextEditorWidget *w) { w->gotoDocumentEnd(); }, true, Tr::tr("Go to Document End"));
    registerAction(GOTO_LINE_START,
            [] (TextEditorWidget *w) { w->gotoLineStart(); }, true, Tr::tr("Go to Line Start"));
    registerAction(GOTO_LINE_END,
            [] (TextEditorWidget *w) { w->gotoLineEnd(); }, true, Tr::tr("Go to Line End"));
    registerAction(GOTO_NEXT_LINE,
            [] (TextEditorWidget *w) { w->gotoNextLine(); }, true, Tr::tr("Go to Next Line"));
    registerAction(GOTO_PREVIOUS_LINE,
            [] (TextEditorWidget *w) { w->gotoPreviousLine(); }, true, Tr::tr("Go to Previous Line"));
    registerAction(GOTO_PREVIOUS_CHARACTER,
            [] (TextEditorWidget *w) { w->gotoPreviousCharacter(); }, true, Tr::tr("Go to Previous Character"));
    registerAction(GOTO_NEXT_CHARACTER,
            [] (TextEditorWidget *w) { w->gotoNextCharacter(); }, true, Tr::tr("Go to Next Character"));
    registerAction(GOTO_PREVIOUS_WORD,
            [] (TextEditorWidget *w) { w->gotoPreviousWord(); }, true, Tr::tr("Go to Previous Word"));
    registerAction(GOTO_NEXT_WORD,
            [] (TextEditorWidget *w) { w->gotoNextWord(); }, true, Tr::tr("Go to Next Word"));
    registerAction(GOTO_PREVIOUS_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->gotoPreviousWordCamelCase(); }, false, Tr::tr("Go to Previous Word Camel Case"));
    registerAction(GOTO_NEXT_WORD_CAMEL_CASE,
            [] (TextEditorWidget *w) { w->gotoNextWordCamelCase(); }, false, Tr::tr("Go to Next Word Camel Case"));

    // register GOTO actions with selection
    registerAction(GOTO_LINE_START_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoLineStartWithSelection(); }, true, Tr::tr("Go to Line Start with Selection"));
    registerAction(GOTO_LINE_END_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoLineEndWithSelection(); }, true, Tr::tr("Go to Line End with Selection"));
    registerAction(GOTO_NEXT_LINE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextLineWithSelection(); }, true, Tr::tr("Go to Next Line with Selection"));
    registerAction(GOTO_PREVIOUS_LINE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousLineWithSelection(); }, true, Tr::tr("Go to Previous Line with Selection"));
    registerAction(GOTO_PREVIOUS_CHARACTER_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousCharacterWithSelection(); }, true, Tr::tr("Go to Previous Character with Selection"));
    registerAction(GOTO_NEXT_CHARACTER_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextCharacterWithSelection(); }, true, Tr::tr("Go to Next Character with Selection"));
    registerAction(GOTO_PREVIOUS_WORD_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousWordWithSelection(); }, true, Tr::tr("Go to Previous Word with Selection"));
    registerAction(GOTO_NEXT_WORD_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextWordWithSelection(); }, true, Tr::tr("Go to Next Word with Selection"));
    registerAction(GOTO_PREVIOUS_WORD_CAMEL_CASE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoPreviousWordCamelCaseWithSelection(); }, false, Tr::tr("Go to Previous Word Camel Case with Selection"));
    registerAction(GOTO_NEXT_WORD_CAMEL_CASE_WITH_SELECTION,
            [] (TextEditorWidget *w) { w->gotoNextWordCamelCaseWithSelection(); }, false, Tr::tr("Go to Next Word Camel Case with Selection"));

    // Collect additional modifying actions so we can check for them inside a readonly file
    // and disable them
    m_modifyingActions << m_autoIndentAction;
    m_modifyingActions << m_autoFormatAction;
    m_modifyingActions << m_unCommentSelectionAction;

    updateOptionalActions();
}

void TextEditorActionHandlerPrivate::updateActions()
{
    bool isWritable = m_currentEditorWidget && !m_currentEditorWidget->isReadOnly();
    for (QAction *a : std::as_const(m_modifyingActions))
        a->setEnabled(isWritable);
    m_unCommentSelectionAction->setEnabled((m_optionalActions & TextEditorActionHandler::UnCommentSelection) && isWritable);
    m_visualizeWhitespaceAction->setEnabled(m_currentEditorWidget);
    if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100) {
        m_textWrappingAction->setEnabled(m_currentEditorWidget);
    } else {
        m_textWrappingAction->setEnabled(false);
        m_textWrappingAction->setChecked(false);
    }
    if (m_currentEditorWidget) {
        m_visualizeWhitespaceAction->setChecked(
                    m_currentEditorWidget->displaySettings().m_visualizeWhitespace);
        m_textWrappingAction->setChecked(m_currentEditorWidget->displaySettings().m_textWrapping);
    }

    bool canRedo = false;
    bool canUndo = false;
    bool canCopy = false;

    if (m_currentEditor && m_currentEditor->document()
        && m_currentEditor->document()->id() == m_editorId) {
        canRedo = m_canRedoCallback ? m_canRedoCallback(m_currentEditor) : false;
        canUndo = m_canUndoCallback ? m_canUndoCallback(m_currentEditor) : false;

        if (m_currentEditorWidget) {
            canRedo = m_canRedoCallback ? canRedo
                                        : m_currentEditorWidget->document()->isRedoAvailable();
            canUndo = m_canUndoCallback ? canUndo
                                        : m_currentEditorWidget->document()->isUndoAvailable();
            canCopy = m_currentEditorWidget->textCursor().hasSelection();
        }
    }

    updateRedoAction(canRedo);
    updateUndoAction(canUndo);
    updateCopyAction(canCopy);

    updateOptionalActions();
}

void TextEditorActionHandlerPrivate::updateOptionalActions()
{
    const uint optionalActions = m_currentEditorWidget ? m_currentEditorWidget->optionalActions()
                                                       : m_optionalActions;
    m_followSymbolAction->setEnabled(
        optionalActions & TextEditorActionHandler::FollowSymbolUnderCursor);
    m_followSymbolInNextSplitAction->setEnabled(
        optionalActions & TextEditorActionHandler::FollowSymbolUnderCursor);
    m_followToTypeAction->setEnabled(
        optionalActions & TextEditorActionHandler::FollowTypeUnderCursor);
    m_followToTypeInNextSplitAction->setEnabled(
        optionalActions & TextEditorActionHandler::FollowTypeUnderCursor);
    m_findUsageAction->setEnabled(
        optionalActions & TextEditorActionHandler::FindUsage);
    m_jumpToFileAction->setEnabled(
        optionalActions & TextEditorActionHandler::JumpToFileUnderCursor);
    m_jumpToFileInNextSplitAction->setEnabled(
        optionalActions & TextEditorActionHandler::JumpToFileUnderCursor);
    m_unfoldAllAction->setEnabled(
        optionalActions & TextEditorActionHandler::UnCollapseAll);
    m_renameSymbolAction->setEnabled(
        optionalActions & TextEditorActionHandler::RenameSymbol);
    m_openCallHierarchyAction->setEnabled(
        optionalActions & TextEditorActionHandler::CallHierarchy);

    bool formatEnabled = (optionalActions & TextEditorActionHandler::Format)
                         && m_currentEditorWidget && !m_currentEditorWidget->isReadOnly();
    m_autoIndentAction->setEnabled(formatEnabled);
    m_autoFormatAction->setEnabled(formatEnabled);
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
    if (m_copyHtmlAction)
        m_copyHtmlAction->setEnabled(hasCopyableText);
}

void TextEditorActionHandlerPrivate::updateCurrentEditor(Core::IEditor *editor)
{
    if (m_currentEditorWidget)
        m_currentEditorWidget->disconnect(this);
    m_currentEditorWidget = nullptr;

    m_currentEditor = editor;

    if (editor && editor->document()->id() == m_editorId) {
        m_currentEditorWidget = m_findTextWidget(editor);
        if (m_currentEditorWidget) {
            connect(m_currentEditorWidget, &QPlainTextEdit::undoAvailable,
                    this, &TextEditorActionHandlerPrivate::updateUndoAction);
            connect(m_currentEditorWidget, &QPlainTextEdit::redoAvailable,
                    this, &TextEditorActionHandlerPrivate::updateRedoAction);
            connect(m_currentEditorWidget, &QPlainTextEdit::copyAvailable,
                    this, &TextEditorActionHandlerPrivate::updateCopyAction);
            connect(m_currentEditorWidget, &TextEditorWidget::readOnlyChanged,
                    this, &TextEditorActionHandlerPrivate::updateActions);
            connect(m_currentEditorWidget, &TextEditorWidget::optionalActionMaskChanged,
                    this, &TextEditorActionHandlerPrivate::updateOptionalActions);
        }
    }
    updateActions();
}

} // namespace Internal

TextEditorActionHandler::TextEditorActionHandler(Utils::Id editorId,
                                                 Utils::Id contextId,
                                                 uint optionalActions,
                                                 const TextEditorWidgetResolver &resolver)
    : d(new Internal::TextEditorActionHandlerPrivate(editorId, contextId, optionalActions))
{
    if (resolver)
        d->m_findTextWidget = resolver;
    else
        d->m_findTextWidget = TextEditorWidget::fromEditor;
}

uint TextEditorActionHandler::optionalActions() const
{
    return d->m_optionalActions;
}

TextEditorActionHandler::~TextEditorActionHandler()
{
    delete d;
}

void TextEditorActionHandler::updateCurrentEditor()
{
    d->updateCurrentEditor(Core::EditorManager::currentEditor());
}

void TextEditorActionHandler::updateActions()
{
    d->updateActions();
}

void TextEditorActionHandler::setCanUndoCallback(const Predicate &callback)
{
    d->m_canUndoCallback = callback;
}

void TextEditorActionHandler::setCanRedoCallback(const Predicate &callback)
{
    d->m_canRedoCallback = callback;
}

void TextEditorActionHandler::setUnhandledCallback(const UnhandledCallback &callback)
{
    d->m_unhandledCallback = callback;
}

} // namespace TextEditor
