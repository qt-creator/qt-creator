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

#include "texteditorplugin.h"

#include "findincurrentfile.h"
#include "findinfiles.h"
#include "findinopenfiles.h"
#include "fontsettings.h"
#include "highlighter.h"
#include "linenumberfilter.h"
#include "outlinefactory.h"
#include "plaintexteditorfactory.h"
#include "snippets/snippetprovider.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditoractionhandler.h"
#include "texteditorsettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/diffservice.h>
#include <coreplugin/externaltoolmanager.h>
#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/macroexpander.h>

#include <QAction>
#include <QDir>

using namespace Core;
using namespace Utils;

namespace TextEditor {
namespace Internal {

static const char kCurrentDocumentSelection[] = "CurrentDocument:Selection";
static const char kCurrentDocumentRow[] = "CurrentDocument:Row";
static const char kCurrentDocumentColumn[] = "CurrentDocument:Column";
static const char kCurrentDocumentRowCount[] = "CurrentDocument:RowCount";
static const char kCurrentDocumentColumnCount[] = "CurrentDocument:ColumnCount";
static const char kCurrentDocumentFontSize[] = "CurrentDocument:FontSize";
static const char kCurrentDocumentWordUnderCursor[] = "CurrentDocument:WordUnderCursor";

class TextEditorPluginPrivate : public QObject
{
public:
    void extensionsInitialized();
    void updateSearchResultsFont(const TextEditor::FontSettings &);
    void updateSearchResultsTabWidth(const TextEditor::TabSettings &tabSettings);
    void updateCurrentSelection(const QString &text);

    void createStandardContextMenu();

    TextEditorSettings settings;
    LineNumberFilter lineNumberFilter; // Goto line functionality for quick open
    OutlineFactory outlineFactory;

    FindInFiles findInFilesFilter;
    FindInCurrentFile findInCurrentFileFilter;
    FindInOpenFiles findInOpenFilesFilter;

    PlainTextEditorFactory plainTextEditorFactory;
};

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

bool TextEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new TextEditorPluginPrivate;

    Context context(TextEditor::Constants::C_TEXTEDITOR);

    // Add shortcut for invoking automatic completion
    QAction *completionAction = new QAction(tr("Trigger Completion"), this);
    Command *command = ActionManager::registerAction(completionAction, Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Space") : tr("Ctrl+Space")));
    connect(completionAction, &QAction::triggered, []() {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->invokeAssist(Completion);
    });
    connect(command, &Command::keySequenceChanged, [command] {
        Utils::FancyLineEdit::setCompletionShortcut(command->keySequence());
    });
    Utils::FancyLineEdit::setCompletionShortcut(command->keySequence());

    // Add shortcut for invoking function hint completion
    QAction *functionHintAction = new QAction(tr("Display Function Hint"), this);
    command = ActionManager::registerAction(functionHintAction, Constants::FUNCTION_HINT, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Shift+D")
                                                                : tr("Ctrl+Shift+D")));
    connect(functionHintAction, &QAction::triggered, []() {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->invokeAssist(FunctionHint);
    });

    // Add shortcut for invoking quick fix options
    QAction *quickFixAction = new QAction(tr("Trigger Refactoring Action"), this);
    Command *quickFixCommand = ActionManager::registerAction(quickFixAction, Constants::QUICKFIX_THIS, context);
    quickFixCommand->setDefaultKeySequence(QKeySequence(tr("Alt+Return")));
    connect(quickFixAction, &QAction::triggered, []() {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->invokeAssist(QuickFix);
    });

    QAction *showContextMenuAction = new QAction(tr("Show Context Menu"), this);
    ActionManager::registerAction(showContextMenuAction,
                                  Constants::SHOWCONTEXTMENU,
                                  context);
    connect(showContextMenuAction, &QAction::triggered, []() {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->showContextMenu();
    });

    // Add text snippet provider.
    SnippetProvider::registerGroup(Constants::TEXT_SNIPPET_GROUP_ID,
                                    tr("Text", "SnippetProvider"));

    d->createStandardContextMenu();

    return true;
}

void TextEditorPluginPrivate::extensionsInitialized()
{
    connect(FolderNavigationWidgetFactory::instance(),
            &FolderNavigationWidgetFactory::aboutToShowContextMenu,
            this,
            [](QMenu *menu, const FilePath &filePath, bool isDir) {
                if (!isDir && Core::DiffService::instance()) {
                    menu->addAction(TextEditor::TextDocument::createDiffAgainstCurrentFileAction(
                        menu, [filePath]() { return filePath; }));
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

    Utils::MacroExpander *expander = Utils::globalMacroExpander();

    expander->registerVariable(kCurrentDocumentSelection,
        tr("Selected text within the current document."),
        []() -> QString {
            QString value;
            if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor()) {
                value = editor->selectedText();
                value.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
            }
            return value;
        });

    expander->registerIntVariable(kCurrentDocumentRow,
        tr("Line number of the text cursor position in current document (starts with 1)."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->currentLine() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentColumn,
        tr("Column number of the text cursor position in current document (starts with 0)."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->currentColumn() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentRowCount,
        tr("Number of lines visible in current document."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->rowCount() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentColumnCount,
        tr("Number of columns visible in current document."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->columnCount() : 0;
        });

    expander->registerIntVariable(kCurrentDocumentFontSize,
        tr("Current document's font size in points."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->widget()->font().pointSize() : 0;
        });

    expander->registerVariable(kCurrentDocumentWordUnderCursor,
                               tr("Word under the current document's text cursor."),
                               []() {
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
        window->setTextEditorFont(QFont(settings.family(), settings.fontSize() * settings.fontZoom() / 100),
            {std::make_pair(SearchResultColor::Style::Default,
             SearchResultColor(textFormat.background(), textFormat.foreground(),
             defaultResultFormat.background(), defaultResultFormat.foreground())),
             std::make_pair(SearchResultColor::Style::Alt1,
                          SearchResultColor(textFormat.background(), textFormat.foreground(),
                          alt1ResultFormat.background(), alt1ResultFormat.foreground())),
             std::make_pair(SearchResultColor::Style::Alt2,
                          SearchResultColor(textFormat.background(), textFormat.foreground(),
                          alt2ResultFormat.background(), alt2ResultFormat.foreground()))});
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

} // namespace Internal
} // namespace TextEditor
