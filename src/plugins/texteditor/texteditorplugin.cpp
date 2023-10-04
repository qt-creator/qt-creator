// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorplugin.h"

#include "bookmarkfilter.h"
#include "bookmarkmanager.h"
#include "findincurrentfile.h"
#include "findinfiles.h"
#include "findinopenfiles.h"
#include "fontsettings.h"
#include "highlighter.h"
#include "icodestylepreferences.h"
#include "jsoneditor.h"
#include "linenumberfilter.h"
#include "markdowneditor.h"
#include "outlinefactory.h"
#include "plaintexteditorfactory.h"
#include "snippets/snippetprovider.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#ifdef WITH_TESTS
#include "codeassist/codeassist_test.h"
#include "highlighter_test.h"
#endif

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/diffservice.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/externaltoolmanager.h>
#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/macroexpander.h>
#include <utils/utilsicons.h>

#include <QMenu>

using namespace Core;
using namespace Utils;
using namespace TextEditor::Constants;

namespace TextEditor::Internal {

const char kCurrentDocumentSelection[] = "CurrentDocument:Selection";
const char kCurrentDocumentRow[] = "CurrentDocument:Row";
const char kCurrentDocumentColumn[] = "CurrentDocument:Column";
const char kCurrentDocumentRowCount[] = "CurrentDocument:RowCount";
const char kCurrentDocumentColumnCount[] = "CurrentDocument:ColumnCount";
const char kCurrentDocumentFontSize[] = "CurrentDocument:FontSize";
const char kCurrentDocumentWordUnderCursor[] = "CurrentDocument:WordUnderCursor";

class TextEditorPluginPrivate : public QObject
{
public:
    TextEditorPluginPrivate();

    void updateActions(bool enableToggle, int stateMask);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);

    void requestContextMenu(TextEditorWidget *widget, int lineNumber, QMenu *menu);

    void extensionsInitialized();
    void updateSearchResultsFont(const FontSettings &);
    void updateSearchResultsTabWidth(const TabSettings &tabSettings);
    void updateCurrentSelection(const QString &text);

    void createStandardContextMenu();

    BookmarkManager m_bookmarkManager;
    BookmarkFilter m_bookmarkFilter{&m_bookmarkManager};
    BookmarkViewFactory m_bookmarkViewFactory{&m_bookmarkManager};

    QAction m_toggleAction{Tr::tr("Toggle Bookmark")};
    QAction m_editAction{Tr::tr("Edit Bookmark")};
    QAction m_prevAction{Tr::tr("Previous Bookmark")};
    QAction m_nextAction{Tr::tr("Next Bookmark")};
    QAction m_docPrevAction{Tr::tr("Previous Bookmark in Document")};
    QAction m_docNextAction{Tr::tr("Next Bookmark in Document")};
    QAction m_editBookmarkAction{Tr::tr("Edit Bookmark")};
    QAction m_bookmarkMarginAction{Tr::tr("Toggle Bookmark")};

    int m_marginActionLineNumber = 0;
    FilePath m_marginActionFileName;

    TextEditorSettings settings;
    LineNumberFilter lineNumberFilter; // Goto line functionality for quick open
    OutlineFactory outlineFactory;

    FindInFiles findInFilesFilter;
    FindInCurrentFile findInCurrentFileFilter;
    FindInOpenFiles findInOpenFilesFilter;

    PlainTextEditorFactory plainTextEditorFactory;
    MarkdownEditorFactory markdownEditorFactory;
    JsonEditorFactory jsonEditorFactory;
};

TextEditorPluginPrivate::TextEditorPluginPrivate()
{
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *touchBar = ActionManager::actionContainer(Core::Constants::TOUCH_BAR);
    ActionContainer *mbm = ActionManager::createMenu(Id("Bookmarks.Menu"));

    mbm->menu()->setTitle(Tr::tr("&Bookmarks"));
    mtools->addMenu(mbm);

    const Context editorManagerContext(Core::Constants::C_EDITORMANAGER);

    // Toggle
    Command *cmd = ActionManager::registerAction(&m_toggleAction, "Bookmarks.Toggle",
                                                 editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+M") : Tr::tr("Ctrl+M")));
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_BOOKMARK.icon());
    mbm->addAction(cmd);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_EDITOR);

    cmd = ActionManager::registerAction(&m_editAction, "Bookmarks.Edit", editorManagerContext);
    cmd->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Meta+Shift+M") : Tr::tr("Ctrl+Shift+M")));
    mbm->addAction(cmd);

    mbm->addSeparator();

    // Previous
    m_prevAction.setIcon(Icons::PREV_TOOLBAR.icon());
    m_prevAction.setIconVisibleInMenu(false);
    cmd = ActionManager::registerAction(&m_prevAction, BOOKMARKS_PREV_ACTION, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+,")
                                                            : Tr::tr("Ctrl+,")));
    mbm->addAction(cmd);

    // Next
    m_nextAction.setIcon(Icons::NEXT_TOOLBAR.icon());
    m_nextAction.setIconVisibleInMenu(false);
    cmd = ActionManager::registerAction(&m_nextAction, BOOKMARKS_NEXT_ACTION, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+.")
                                                            : Tr::tr("Ctrl+.")));
    mbm->addAction(cmd);

    mbm->addSeparator();

    // Previous Doc
    cmd = ActionManager::registerAction(&m_docPrevAction, "Bookmarks.PreviousDocument",
                                        editorManagerContext);
    mbm->addAction(cmd);

    // Next Doc
    cmd = ActionManager::registerAction(&m_docNextAction, "Bookmarks.NextDocument",
                                        editorManagerContext);
    mbm->addAction(cmd);

    connect(&m_toggleAction, &QAction::triggered, this, [this] {
        IEditor *editor = EditorManager::currentEditor();
        auto widget = TextEditorWidget::fromEditor(editor);
        if (widget && editor && !editor->document()->isTemporary())
            m_bookmarkManager.toggleBookmark(editor->document()->filePath(), editor->currentLine());
    });

    connect(&m_editAction, &QAction::triggered, this, [this] {
        IEditor *editor = EditorManager::currentEditor();
        auto widget = TextEditorWidget::fromEditor(editor);
        if (widget && editor && !editor->document()->isTemporary()) {
            const FilePath filePath = editor->document()->filePath();
            const int line = editor->currentLine();
            if (!m_bookmarkManager.hasBookmarkInPosition(filePath, line))
                m_bookmarkManager.toggleBookmark(filePath, line);
            m_bookmarkManager.editByFileAndLine(filePath, line);
        }
    });

    connect(&m_prevAction, &QAction::triggered, &m_bookmarkManager, &BookmarkManager::prev);
    connect(&m_nextAction, &QAction::triggered, &m_bookmarkManager, &BookmarkManager::next);
    connect(&m_docPrevAction, &QAction::triggered,
            &m_bookmarkManager, &BookmarkManager::prevInDocument);
    connect(&m_docNextAction, &QAction::triggered,
            &m_bookmarkManager, &BookmarkManager::nextInDocument);

    connect(&m_editBookmarkAction, &QAction::triggered, this, [this] {
            m_bookmarkManager.editByFileAndLine(m_marginActionFileName, m_marginActionLineNumber);
    });

    connect(&m_bookmarkManager, &BookmarkManager::updateActions,
            this, &TextEditorPluginPrivate::updateActions);
    updateActions(false, m_bookmarkManager.state());

    connect(&m_bookmarkMarginAction, &QAction::triggered, this, [this] {
            m_bookmarkManager.toggleBookmark(m_marginActionFileName, m_marginActionLineNumber);
    });

    // EditorManager
    connect(EditorManager::instance(), &EditorManager::editorAboutToClose,
        this, &TextEditorPluginPrivate::editorAboutToClose);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
        this, &TextEditorPluginPrivate::editorOpened);
}

void TextEditorPluginPrivate::updateActions(bool enableToggle, int state)
{
    const bool hasbm    = state >= BookmarkManager::HasBookMarks;
    const bool hasdocbm = state == BookmarkManager::HasBookmarksInDocument;

    m_toggleAction.setEnabled(enableToggle);
    m_editAction.setEnabled(enableToggle);
    m_prevAction.setEnabled(hasbm);
    m_nextAction.setEnabled(hasbm);
    m_docPrevAction.setEnabled(hasdocbm);
    m_docNextAction.setEnabled(hasdocbm);
}

void TextEditorPluginPrivate::editorOpened(IEditor *editor)
{
    if (auto widget = TextEditorWidget::fromEditor(editor)) {
        connect(widget, &TextEditorWidget::markRequested,
                this, [this, editor](TextEditorWidget *, int line, TextMarkRequestKind kind) {
                    if (kind == BookmarkRequest && !editor->document()->isTemporary())
                        m_bookmarkManager.toggleBookmark(editor->document()->filePath(), line);
                });

        connect(widget, &TextEditorWidget::markContextMenuRequested,
                this, &TextEditorPluginPrivate::requestContextMenu);
    }
}

void TextEditorPluginPrivate::editorAboutToClose(IEditor *editor)
{
    if (auto widget = TextEditorWidget::fromEditor(editor)) {
        disconnect(widget, &TextEditorWidget::markContextMenuRequested,
                   this, &TextEditorPluginPrivate::requestContextMenu);
    }
}

void TextEditorPluginPrivate::requestContextMenu(TextEditorWidget *widget,
    int lineNumber, QMenu *menu)
{
    if (widget->textDocument()->isTemporary())
        return;

    m_marginActionLineNumber = lineNumber;
    m_marginActionFileName = widget->textDocument()->filePath();

    menu->addAction(&m_bookmarkMarginAction);
    if (m_bookmarkManager.hasBookmarkInPosition(m_marginActionFileName, m_marginActionLineNumber))
        menu->addAction(&m_editBookmarkAction);
}

static TextEditorPlugin *m_instance = nullptr;

TextEditorPlugin::TextEditorPlugin()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
}

TextEditorPlugin::~TextEditorPlugin()
{
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

TextEditorPlugin *TextEditorPlugin::instance()
{
    return m_instance;
}

void TextEditorPlugin::initialize()
{
    d = new TextEditorPluginPrivate;

    Context context(TextEditor::Constants::C_TEXTEDITOR);

    // Add shortcut for invoking automatic completion
    QAction *completionAction = new QAction(Tr::tr("Trigger Completion"), this);
    Command *command = ActionManager::registerAction(completionAction, Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Space") : Tr::tr("Ctrl+Space")));
    connect(completionAction, &QAction::triggered, this, [] {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->invokeAssist(Completion);
    });
    connect(command, &Command::keySequenceChanged, [command] {
        FancyLineEdit::setCompletionShortcut(command->keySequence());
    });
    FancyLineEdit::setCompletionShortcut(command->keySequence());

    // Add shortcut for invoking function hint completion
    QAction *functionHintAction = new QAction(Tr::tr("Display Function Hint"), this);
    command = ActionManager::registerAction(functionHintAction, Constants::FUNCTION_HINT, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Shift+D")
                                                                : Tr::tr("Ctrl+Shift+D")));
    connect(functionHintAction, &QAction::triggered, this, [] {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->invokeAssist(FunctionHint);
    });

    // Add shortcut for invoking quick fix options
    QAction *quickFixAction = new QAction(Tr::tr("Trigger Refactoring Action"), this);
    Command *quickFixCommand = ActionManager::registerAction(quickFixAction, Constants::QUICKFIX_THIS, context);
    quickFixCommand->setDefaultKeySequence(QKeySequence(Tr::tr("Alt+Return")));
    connect(quickFixAction, &QAction::triggered, this, [] {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->invokeAssist(QuickFix);
    });

    QAction *showContextMenuAction = new QAction(Tr::tr("Show Context Menu"), this);
    ActionManager::registerAction(showContextMenuAction,
                                  Constants::SHOWCONTEXTMENU,
                                  context);
    connect(showContextMenuAction, &QAction::triggered, this, [] {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->showContextMenu();
    });

    // Add text snippet provider.
    SnippetProvider::registerGroup(Constants::TEXT_SNIPPET_GROUP_ID,
                                    Tr::tr("Text", "SnippetProvider"));

    d->createStandardContextMenu();

#ifdef WITH_TESTS
    addTest<CodeAssistTests>();
    addTest<GenerigHighlighterTests>();
#endif
}

void TextEditorPluginPrivate::extensionsInitialized()
{
    connect(FolderNavigationWidgetFactory::instance(),
            &FolderNavigationWidgetFactory::aboutToShowContextMenu,
            this, [](QMenu *menu, const FilePath &filePath, bool isDir) {
                if (!isDir && Core::DiffService::instance()) {
                    menu->addAction(TextDocument::createDiffAgainstCurrentFileAction(
                        menu, [filePath] { return filePath; }));
                }
            });

    connect(&settings,
            &TextEditorSettings::fontSettingsChanged,
            this,
            &TextEditorPluginPrivate::updateSearchResultsFont);

    updateSearchResultsFont(TextEditorSettings::fontSettings());

    connect(TextEditorSettings::codeStyle(), &ICodeStylePreferences::currentTabSettingsChanged,
            this, &TextEditorPluginPrivate::updateSearchResultsTabWidth);

    updateSearchResultsTabWidth(TextEditorSettings::codeStyle()->currentTabSettings());

    connect(ExternalToolManager::instance(), &ExternalToolManager::replaceSelectionRequested,
            this, &TextEditorPluginPrivate::updateCurrentSelection);
}

void TextEditorPlugin::extensionsInitialized()
{
    d->extensionsInitialized();

    MacroExpander *expander = Utils::globalMacroExpander();

    expander->registerVariable(kCurrentDocumentSelection,
        Tr::tr("Selected text within the current document."),
        []() -> QString {
            QString value;
            if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor()) {
                value = editor->selectedText();
                value.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
            }
            return value;
        });

    expander->registerIntVariable(kCurrentDocumentRow,
        Tr::tr("Line number of the text cursor position in current document (starts with 1)."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->currentLine() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentColumn,
        Tr::tr("Column number of the text cursor position in current document (starts with 0)."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->currentColumn() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentRowCount,
        Tr::tr("Number of lines visible in current document."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->rowCount() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentColumnCount,
        Tr::tr("Number of columns visible in current document."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->columnCount() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentFontSize,
        Tr::tr("Current document's font size in points."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->widget()->font().pointSize() : 0;
        });

    expander->registerVariable(kCurrentDocumentWordUnderCursor,
                               Tr::tr("Word under the current document's text cursor."), [] {
                                   BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
                                   if (!editor)
                                       return QString();
                                   return Text::wordUnderCursor(editor->editorWidget()->textCursor());
                               });
}

LineNumberFilter *TextEditorPlugin::lineNumberFilter()
{
    return &m_instance->d->lineNumberFilter;
}

ExtensionSystem::IPlugin::ShutdownFlag TextEditorPlugin::aboutToShutdown()
{
    Highlighter::handleShutdown();
    return SynchronousShutdown;
}

void TextEditorPluginPrivate::updateSearchResultsFont(const FontSettings &settings)
{
    if (auto window = SearchResultWindow::instance()) {
        const Format textFormat = settings.formatFor(C_TEXT);
        const Format defaultResultFormat = settings.formatFor(C_SEARCH_RESULT);
        const Format alt1ResultFormat = settings.formatFor(C_SEARCH_RESULT_ALT1);
        const Format alt2ResultFormat = settings.formatFor(C_SEARCH_RESULT_ALT2);
        const Format containingFunctionResultFormat =
             settings.formatFor(C_SEARCH_RESULT_CONTAINING_FUNCTION);
        window->setTextEditorFont(QFont(settings.family(), settings.fontSize() * settings.fontZoom() / 100),
            {{SearchResultColor::Style::Default,
              {textFormat.background(), textFormat.foreground(),
               defaultResultFormat.background(), defaultResultFormat.foreground(),
               containingFunctionResultFormat.background(),
               containingFunctionResultFormat.foreground()}},
             {SearchResultColor::Style::Alt1,
              {textFormat.background(), textFormat.foreground(),
               alt1ResultFormat.background(), alt1ResultFormat.foreground(),
               containingFunctionResultFormat.background(),
               containingFunctionResultFormat.foreground()}},
             {SearchResultColor::Style::Alt2,
              {textFormat.background(), textFormat.foreground(),
               alt2ResultFormat.background(), alt2ResultFormat.foreground(),
               containingFunctionResultFormat.background(),
               containingFunctionResultFormat.foreground()}}});
    }
}

void TextEditorPluginPrivate::updateSearchResultsTabWidth(const TabSettings &tabSettings)
{
    if (auto window = SearchResultWindow::instance())
        window->setTabWidth(tabSettings.m_tabSize);
}

void TextEditorPluginPrivate::updateCurrentSelection(const QString &text)
{
    if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor()) {
        const int pos = editor->position();
        int anchor = editor->position(AnchorPosition);
        if (anchor < 0) // no selection
            anchor = pos;
        int selectionLength = pos - anchor;
        const bool selectionInTextDirection = selectionLength >= 0;
        if (!selectionInTextDirection)
            selectionLength = -selectionLength;
        const int start = qMin(pos, anchor);
        editor->setCursorPosition(start);
        editor->replace(selectionLength, text);
        const int replacementEnd = editor->position();
        editor->setCursorPosition(selectionInTextDirection ? start : replacementEnd);
        editor->select(selectionInTextDirection ? replacementEnd : start);
    }
}

void TextEditorPluginPrivate::createStandardContextMenu()
{
    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_STANDARDCONTEXTMENU);
    contextMenu->appendGroup(Constants::G_UNDOREDO);
    contextMenu->appendGroup(Constants::G_COPYPASTE);
    contextMenu->appendGroup(Constants::G_SELECT);
    contextMenu->appendGroup(Constants::G_BOM);

    const auto add = [contextMenu](const Id &id, const Id &group) {
        Command *cmd = ActionManager::command(id);
        if (cmd)
            contextMenu->addAction(cmd, group);
    };

    add(Core::Constants::UNDO, Constants::G_UNDOREDO);
    add(Core::Constants::REDO, Constants::G_UNDOREDO);
    contextMenu->addSeparator(Constants::G_COPYPASTE);
    add(Core::Constants::CUT, Constants::G_COPYPASTE);
    add(Core::Constants::COPY, Constants::G_COPYPASTE);
    add(Core::Constants::PASTE, Constants::G_COPYPASTE);
    add(Constants::CIRCULAR_PASTE, Constants::G_COPYPASTE);
    contextMenu->addSeparator(Constants::G_SELECT);
    add(Core::Constants::SELECTALL, Constants::G_SELECT);
    contextMenu->addSeparator(Constants::G_BOM);
    add(Constants::SWITCH_UTF8BOM, Constants::G_BOM);
}

} // namespace TextEditor::Internal
