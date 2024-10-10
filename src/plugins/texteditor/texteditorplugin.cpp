// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettings.h"
#include "bookmarkfilter.h"
#include "bookmarkmanager.h"
#include "extraencodingsettings.h"
#include "findincurrentfile.h"
#include "findinfiles.h"
#include "findinopenfiles.h"
#include "fontsettings.h"
#include "highlighterhelper.h"
#include "icodestylepreferences.h"
#include "jsoneditor.h"
#include "linenumberfilter.h"
#include "markdowneditor.h"
#include "outlinefactory.h"
#include "plaintexteditorfactory.h"
#include "snippets/snippetprovider.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditor_test.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"
#include "textmark.h"
#include "typehierarchy.h"
#include "typingsettings.h"

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
#include <extensionsystem/iplugin.h>

#include <utils/fancylineedit.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>
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

class TextEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "TextEditor.json")

public:
    ShutdownFlag aboutToShutdown() final;

    void initialize() final;
    void extensionsInitialized() final;

    void updateSearchResultsFont(const FontSettings &);
    void updateSearchResultsTabWidth(const TabSettings &tabSettings);
    void updateCurrentSelection(const QString &text);

    void createStandardContextMenu();
    void createEditorCommands();
};

void TextEditorPlugin::initialize()
{
#ifdef WITH_TESTS
    addTestCreator(createFormatTextTest);
    addTestCreator(createTextDocumentTest);
    addTestCreator(createTextEditorTest);
    addTestCreator(createSnippetParserTest);
#endif

    setupBehaviorSettings();
    setupExtraEncodingSettings();
    setupStorageSettings();
    setupTypingSettings();
    // Currently needed after the previous four lines.
    // FIXME: This kind of dependency should not exist.
    setupTextEditorSettings();

    TabSettings::setRetriever(
        [](const FilePath &) { return TextEditorSettings::codeStyle()->tabSettings(); });

    setupTextMarkRegistry(this);
    setupOutlineFactory();
    setupTypeHierarchyFactory();
    setupLineNumberFilter(); // Goto line functionality for quick open

    setupPlainTextEditor();

    setupBookmarkManager(this);
    setupBookmarkView();
    setupBookmarkFilter();

    setupFindInFiles(this);
    setupFindInCurrentFile();
    setupFindInOpenFiles();

    setupMarkdownEditor();
    setupJsonEditor();

    // Add text snippet provider.
    SnippetProvider::registerGroup(Constants::TEXT_SNIPPET_GROUP_ID,
                                    Tr::tr("Text", "SnippetProvider"));

    createStandardContextMenu();
    createEditorCommands();

#ifdef WITH_TESTS
    addTestCreator(createCodeAssistTests);
    addTestCreator(createGenericHighlighterTests);
#endif

    Utils::Text::setCodeHighlighter(HighlighterHelper::highlightCode);
}

void TextEditorPlugin::extensionsInitialized()
{
    connect(FolderNavigationWidgetFactory::instance(),
            &FolderNavigationWidgetFactory::aboutToShowContextMenu,
            this, [](QMenu *menu, const FilePath &filePath, bool isDir) {
                if (!isDir && Core::DiffService::instance()) {
                    menu->addAction(TextDocument::createDiffAgainstCurrentFileAction(
                        menu, [filePath] { return filePath; }));
                }
            });

    connect(&textEditorSettings(), &TextEditorSettings::fontSettingsChanged,
            this, &TextEditorPlugin::updateSearchResultsFont);

    updateSearchResultsFont(TextEditorSettings::fontSettings());

    connect(TextEditorSettings::codeStyle(), &ICodeStylePreferences::currentTabSettingsChanged,
            this, &TextEditorPlugin::updateSearchResultsTabWidth);

    updateSearchResultsTabWidth(TextEditorSettings::codeStyle()->currentTabSettings());

    connect(ExternalToolManager::instance(), &ExternalToolManager::replaceSelectionRequested,
            this, &TextEditorPlugin::updateCurrentSelection);

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

ExtensionSystem::IPlugin::ShutdownFlag TextEditorPlugin::aboutToShutdown()
{
    HighlighterHelper::handleShutdown();
    return SynchronousShutdown;
}

void TextEditorPlugin::updateSearchResultsFont(const FontSettings &settings)
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

void TextEditorPlugin::updateSearchResultsTabWidth(const TabSettings &tabSettings)
{
    if (auto window = SearchResultWindow::instance())
        window->setTabWidth(tabSettings.m_tabSize);
}

void TextEditorPlugin::updateCurrentSelection(const QString &text)
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

void TextEditorPlugin::createStandardContextMenu()
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

class TextActionBuilder : public ActionBuilder
{
public:
    TextActionBuilder(QObject *contextActionParent, const Utils::Id actionId)
        : ActionBuilder(contextActionParent, actionId)
    {
        setContext(Context(Constants::C_TEXTEDITOR));
    }
};

void TextEditorPlugin::createEditorCommands()
{
    using namespace Core::Constants;
    // Add shortcut for invoking automatic completion
    Command *command = nullptr;
    TextActionBuilder(this, Constants::COMPLETE_THIS)
        .setText(Tr::tr("Trigger Completion"))
        .bindCommand(&command)
        .setDefaultKeySequence(Tr::tr("Meta+Space"), Tr::tr("Ctrl+Space"));

    connect(command, &Command::keySequenceChanged, [command] {
        FancyLineEdit::setCompletionShortcut(command->keySequence());
    });
    FancyLineEdit::setCompletionShortcut(command->keySequence());

    // Add shortcut for invoking function hint completion
    TextActionBuilder(this, Constants::FUNCTION_HINT)
        .setText(Tr::tr("Display Function Hint"))
        .setDefaultKeySequence(Tr::tr("Meta+Shift+D"), Tr::tr("Ctrl+Shift+D"));

    // Add shortcut for invoking quick fix options
    TextActionBuilder(this, Constants::QUICKFIX_THIS)
        .setText(Tr::tr("Trigger Refactoring Action"))
        .setDefaultKeySequence(Tr::tr("Alt+Return"));

    TextActionBuilder(this, Constants::SHOWCONTEXTMENU)
        .setText(Tr::tr("Show Context Menu"));

    TextActionBuilder(this, DELETE_LINE).setText(Tr::tr("Delete &Line"));
    TextActionBuilder(this, DELETE_END_OF_LINE).setText(Tr::tr("Delete Line from Cursor On"));
    TextActionBuilder(this, DELETE_END_OF_WORD).setText(Tr::tr("Delete Word from Cursor On"));
    TextActionBuilder(this, DELETE_END_OF_WORD_CAMEL_CASE)
        .setText(Tr::tr("Delete Word Camel Case from Cursor On"));
    TextActionBuilder(this, DELETE_START_OF_LINE)
        .setText(Tr::tr("Delete Line up to Cursor"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Backspace"), {});
    TextActionBuilder(this, DELETE_START_OF_WORD).setText(Tr::tr("Delete Word up to Cursor"));
    TextActionBuilder(this, DELETE_START_OF_WORD_CAMEL_CASE)
        .setText(Tr::tr("Delete Word Camel Case up to Cursor"));
    TextActionBuilder(this, GOTO_BLOCK_START_WITH_SELECTION)
        .setText(Tr::tr("Go to Block Start with Selection"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+{")));
    TextActionBuilder(this, GOTO_BLOCK_END_WITH_SELECTION)
        .setText(Tr::tr("Go to Block End with Selection"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+}")));
    TextActionBuilder(this, MOVE_LINE_UP)
        .setText(Tr::tr("Move Line Up"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+Up")));
    TextActionBuilder(this, MOVE_LINE_DOWN)
        .setText(Tr::tr("Move Line Down"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+Down")));
    TextActionBuilder(this, COPY_LINE_UP)
        .setText(Tr::tr("Copy Line Up"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+Up")));
    TextActionBuilder(this, COPY_LINE_DOWN)
        .setText(Tr::tr("Copy Line Down"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+Down")));
    TextActionBuilder(this, JOIN_LINES)
        .setText(Tr::tr("Join Lines"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+J")));
    TextActionBuilder(this, INSERT_LINE_ABOVE)
        .setText(Tr::tr("Insert Line Above Current Line"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+Return")));
    TextActionBuilder(this, INSERT_LINE_BELOW)
        .setText(Tr::tr("Insert Line Below Current Line"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Return")));
    TextActionBuilder(this, SWITCH_UTF8BOM).setText(Tr::tr("Toggle UTF-8 BOM"));
    TextActionBuilder(this, INDENT).setText(Tr::tr("Indent"));
    TextActionBuilder(this, UNINDENT).setText(Tr::tr("Unindent"));
    TextActionBuilder(this, FOLLOW_SYMBOL_UNDER_CURSOR)
        .setText(Tr::tr("Follow Symbol Under Cursor"))
        .setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    TextActionBuilder(this, FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT)
        .setText(Tr::tr("Follow Symbol Under Cursor in Next Split"))
        .setDefaultKeySequence(Tr::tr("Meta+E, F2"), Tr::tr("Ctrl+E, F2"));
    TextActionBuilder(this, FOLLOW_SYMBOL_TO_TYPE)
        .setText(Tr::tr("Follow Type Under Cursor"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+F2")));
    TextActionBuilder(this, FOLLOW_SYMBOL_TO_TYPE_IN_NEXT_SPLIT)
        .setText(Tr::tr("Follow Type Under Cursor in Next Split"))
        .setDefaultKeySequence(Tr::tr("Meta+E, Shift+F2"), Tr::tr("Ctrl+E, Ctrl+Shift+F2"));
    TextActionBuilder(this, FIND_USAGES)
        .setText(Tr::tr("Find References to Symbol Under Cursor"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+U")));
    TextActionBuilder(this, RENAME_SYMBOL)
        .setText(Tr::tr("Rename Symbol Under Cursor"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+R")));
    TextActionBuilder(this, JUMP_TO_FILE_UNDER_CURSOR)
        .setText(Tr::tr("Jump to File Under Cursor"))
        .setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    TextActionBuilder(this, JUMP_TO_FILE_UNDER_CURSOR_IN_NEXT_SPLIT)
        .setText(Tr::tr("Jump to File Under Cursor in Next Split"))
        .setDefaultKeySequence(Tr::tr("Meta+E, F2"), Tr::tr("Ctrl+E, F2"));
    TextActionBuilder(this, OPEN_CALL_HIERARCHY).setText(Tr::tr("Open Call Hierarchy"));
    TextActionBuilder(this, OPEN_TYPE_HIERARCHY)
        .setText(Tr::tr("Open Type Hierarchy"))
        .setDefaultKeySequence(Tr::tr("Meta+Shift+T"), Tr::tr("Ctrl+Shift+T"));
    TextActionBuilder(this, VIEW_PAGE_UP)
        .setText(Tr::tr("Move the View a Page Up and Keep the Cursor Position"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+PgUp")));
    TextActionBuilder(this, VIEW_PAGE_DOWN)
        .setText(Tr::tr("Move the View a Page Down and Keep the Cursor Position"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+PgDown")));
    TextActionBuilder(this, VIEW_LINE_UP)
        .setText(Tr::tr("Move the View a Line Up and Keep the Cursor Position"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Up")));
    TextActionBuilder(this, VIEW_LINE_DOWN)
        .setText(Tr::tr("Move the View a Line Down and Keep the Cursor Position"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Down")));

    ActionManager::actionContainer(M_EDIT);
    TextActionBuilder(this, SELECT_ENCODING)
        .setText(Tr::tr("Select Encoding..."))
        .addToContainer(M_EDIT, G_EDIT_OTHER);
    TextActionBuilder(this, CIRCULAR_PASTE)
        .setText(Tr::tr("Paste from Clipboard History"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+V")))
        .addToContainer(M_EDIT, G_EDIT_COPYPASTE);
    TextActionBuilder(this, NO_FORMAT_PASTE)
        .setText(Tr::tr("Paste Without Formatting"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Alt+Shift+V"), QString())
        .addToContainer(M_EDIT, G_EDIT_COPYPASTE);

    ActionManager::actionContainer(M_EDIT_ADVANCED);
    TextActionBuilder(this, AUTO_INDENT_SELECTION)
        .setText(Tr::tr("Auto-&indent Selection"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+I")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FORMAT);
    TextActionBuilder(this, AUTO_FORMAT_SELECTION)
        .setText(Tr::tr("Auto-&format Selection"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+;")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FORMAT);
    TextActionBuilder(this, REWRAP_PARAGRAPH)
        .setText(Tr::tr("&Rewrap Paragraph"))
        .setDefaultKeySequence(Tr::tr("Meta+E, R"), Tr::tr("Ctrl+E, R"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FORMAT);
    TextActionBuilder(this, VISUALIZE_WHITESPACE)
        .setText(Tr::tr("&Visualize Whitespace"))
        .setDefaultKeySequence(Tr::tr("Meta+E, Meta+V"), Tr::tr("Ctrl+E, Ctrl+V"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FORMAT)
        .setCheckable(true);
    TextActionBuilder(this, CLEAN_WHITESPACE)
        .setText(Tr::tr("Clean Whitespace"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FORMAT);
    TextActionBuilder(this, TEXT_WRAPPING)
        .setText(Tr::tr("Enable Text &Wrapping"))
        .setDefaultKeySequence(Tr::tr("Meta+E, Meta+W"), Tr::tr("Ctrl+E, Ctrl+W"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FORMAT)
        .setCheckable(true);
    TextActionBuilder(this, UN_COMMENT_SELECTION)
        .setText(Tr::tr("Toggle Comment &Selection"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+/")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FORMAT);
    TextActionBuilder(this, CUT_LINE)
        .setText(Tr::tr("Cut &Line"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Shift+Del")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, COPY_LINE)
        .setText(Tr::tr("Copy &Line"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Ins")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, COPY_WITH_HTML)
        .setText(Tr::tr("Copy With Highlighting"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, ADD_CURSORS_TO_LINE_ENDS)
        .setText(Tr::tr("Create Cursors at Selected Line Ends"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Alt+Shift+I")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, ADD_SELECT_NEXT_FIND_MATCH)
        .setText(Tr::tr("Add Next Occurrence to Selection"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+D")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, DUPLICATE_SELECTION)
        .setText(Tr::tr("&Duplicate Selection"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, DUPLICATE_SELECTION_AND_COMMENT)
        .setText(Tr::tr("&Duplicate Selection and Comment"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, UPPERCASE_SELECTION)
        .setText(Tr::tr("Uppercase Selection"))
        .setDefaultKeySequence(Tr::tr("Meta+Shift+U"), Tr::tr("Alt+Shift+U"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, LOWERCASE_SELECTION)
        .setText(Tr::tr("Lowercase Selection"))
        .setDefaultKeySequence(Tr::tr("Meta+U"), Tr::tr("Alt+U"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, SORT_LINES)
        .setText(Tr::tr("Sort Lines"))
        .setDefaultKeySequence(Tr::tr("Meta+Shift+S"), Tr::tr("Alt+Shift+S"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_TEXT);
    TextActionBuilder(this, FOLD)
        .setText(Tr::tr("Fold"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+<")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_COLLAPSING);
    TextActionBuilder(this, UNFOLD)
        .setText(Tr::tr("Unfold"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+>")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_COLLAPSING);
    TextActionBuilder(this, FOLD_RECURSIVELY)
        .setText(Tr::tr("Fold Recursively"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_COLLAPSING);
    TextActionBuilder(this, UNFOLD_RECURSIVELY)
        .setText(Tr::tr("Unfold Recursively"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_COLLAPSING);
    TextActionBuilder(this, UNFOLD_ALL)
        .setText(Tr::tr("Toggle &Fold All"))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_COLLAPSING);
    TextActionBuilder(this, INCREASE_FONT_SIZE)
        .setText(Tr::tr("Increase Font Size"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl++")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FONT);
    TextActionBuilder(this, DECREASE_FONT_SIZE)
        .setText(Tr::tr("Decrease Font Size"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+-")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FONT);
    TextActionBuilder(this, RESET_FONT_SIZE)
        .setText(Tr::tr("Reset Font Size"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+0")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_FONT);
    TextActionBuilder(this, GOTO_BLOCK_START)
        .setText(Tr::tr("Go to Block Start"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+[")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_BLOCKS);
    TextActionBuilder(this, GOTO_BLOCK_END)
        .setText(Tr::tr("Go to Block End"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+]")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_BLOCKS);
    TextActionBuilder(this, SELECT_BLOCK_UP)
        .setText(Tr::tr("Select Block Up"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+U")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_BLOCKS);
    TextActionBuilder(this, SELECT_BLOCK_DOWN)
        .setText(Tr::tr("Select Block Down"))
        .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+Alt+U")))
        .addToContainer(M_EDIT_ADVANCED, G_EDIT_BLOCKS);
    TextActionBuilder(this, SELECT_WORD_UNDER_CURSOR).setText(Tr::tr("Select Word Under Cursor"));
    TextActionBuilder(this, CLEAR_SELECTION)
        .setText(QCoreApplication::translate("QtC::Terminal", "Clear Selection"));

    TextActionBuilder(this, GOTO_DOCUMENT_START).setText(Tr::tr("Go to Document Start"));
    TextActionBuilder(this, GOTO_DOCUMENT_END).setText(Tr::tr("Go to Document End"));
    TextActionBuilder(this, GOTO_LINE_START).setText(Tr::tr("Go to Line Start"));
    TextActionBuilder(this, GOTO_LINE_END).setText(Tr::tr("Go to Line End"));
    TextActionBuilder(this, GOTO_NEXT_LINE).setText(Tr::tr("Go to Next Line"));
    TextActionBuilder(this, GOTO_PREVIOUS_LINE).setText(Tr::tr("Go to Previous Line"));
    TextActionBuilder(this, GOTO_PREVIOUS_CHARACTER).setText(Tr::tr("Go to Previous Character"));
    TextActionBuilder(this, GOTO_NEXT_CHARACTER).setText(Tr::tr("Go to Next Character"));
    TextActionBuilder(this, GOTO_PREVIOUS_WORD).setText(Tr::tr("Go to Previous Word"));
    TextActionBuilder(this, GOTO_NEXT_WORD).setText(Tr::tr("Go to Next Word"));
    TextActionBuilder(this, GOTO_PREVIOUS_WORD_CAMEL_CASE)
        .setText(Tr::tr("Go to Previous Word (Camel Case)"));
    TextActionBuilder(this, GOTO_NEXT_WORD_CAMEL_CASE).setText(Tr::tr("Go to Next Word (Camel Case)"));

    TextActionBuilder(this, GOTO_LINE_START_WITH_SELECTION)
        .setText(Tr::tr("Go to Line Start with Selection"));
    TextActionBuilder(this, GOTO_LINE_END_WITH_SELECTION)
        .setText(Tr::tr("Go to Line End with Selection"));
    TextActionBuilder(this, GOTO_NEXT_LINE_WITH_SELECTION)
        .setText(Tr::tr("Go to Next Line with Selection"));
    TextActionBuilder(this, GOTO_PREVIOUS_LINE_WITH_SELECTION)
        .setText(Tr::tr("Go to Previous Line with Selection"));
    TextActionBuilder(this, GOTO_PREVIOUS_CHARACTER_WITH_SELECTION)
        .setText(Tr::tr("Go to Previous Character with Selection"));
    TextActionBuilder(this, GOTO_NEXT_CHARACTER_WITH_SELECTION)
        .setText(Tr::tr("Go to Next Character with Selection"));
    TextActionBuilder(this, GOTO_PREVIOUS_WORD_WITH_SELECTION)
        .setText(Tr::tr("Go to Previous Word with Selection"));
    TextActionBuilder(this, GOTO_NEXT_WORD_WITH_SELECTION)
        .setText(Tr::tr("Go to Next Word with Selection"));
    TextActionBuilder(this, GOTO_PREVIOUS_WORD_CAMEL_CASE_WITH_SELECTION)
        .setText(Tr::tr("Go to Previous Word (Camel Case) with Selection"));
    TextActionBuilder(this, GOTO_NEXT_WORD_CAMEL_CASE_WITH_SELECTION)
        .setText(Tr::tr("Go to Next Word (Camel Case) with Selection"));
}

} // namespace TextEditor::Internal

#include "texteditorplugin.moc"
