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

#include "texteditor.h"
#include "findincurrentfile.h"
#include "findinfiles.h"
#include "findinopenfiles.h"
#include "fontsettings.h"
#include "generichighlighter/manager.h"
#include "linenumberfilter.h"
#include "outlinefactory.h"
#include "plaintexteditorfactory.h"
#include "snippets/snippetprovider.h"
#include "texteditoractionhandler.h"
#include "texteditorsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/externaltoolmanager.h>
#include <extensionsystem/pluginmanager.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <utils/qtcassert.h>
#include <utils/macroexpander.h>

#include <QtPlugin>
#include <QAction>
#include <QDir>

using namespace Core;

namespace TextEditor {
namespace Internal {

static const char kCurrentDocumentSelection[] = "CurrentDocument:Selection";
static const char kCurrentDocumentRow[] = "CurrentDocument:Row";
static const char kCurrentDocumentColumn[] = "CurrentDocument:Column";
static const char kCurrentDocumentRowCount[] = "CurrentDocument:RowCount";
static const char kCurrentDocumentColumnCount[] = "CurrentDocument:ColumnCount";
static const char kCurrentDocumentFontSize[] = "CurrentDocument:FontSize";

static TextEditorPlugin *m_instance = 0;

TextEditorPlugin::TextEditorPlugin()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
}

TextEditorPlugin::~TextEditorPlugin()
{
    m_instance = 0;
}

TextEditorPlugin *TextEditorPlugin::instance()
{
    return m_instance;
}

// ExtensionSystem::PluginInterface
bool TextEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    m_settings = new TextEditorSettings(this);

    // Add plain text editor factory
    addAutoReleasedObject(new PlainTextEditorFactory);

    // Goto line functionality for quick open
    m_lineNumberFilter = new LineNumberFilter;
    addAutoReleasedObject(m_lineNumberFilter);

    Context context(TextEditor::Constants::C_TEXTEDITOR);

    // Add shortcut for invoking automatic completion
    QAction *completionAction = new QAction(tr("Trigger Completion"), this);
    Command *command = ActionManager::registerAction(completionAction, Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Space") : tr("Ctrl+Space")));
    connect(completionAction, &QAction::triggered, []() {
        if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
            editor->editorWidget()->invokeAssist(Completion);
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

    // Generic highlighter.
    connect(ICore::instance(), &ICore::coreOpened, Manager::instance(), &Manager::registerHighlightingFiles);

    // Add text snippet provider.
    SnippetProvider::registerGroup(Constants::TEXT_SNIPPET_GROUP_ID,
                                    tr("Text", "SnippetProvider"));

    m_outlineFactory = new OutlineFactory;
    addAutoReleasedObject(m_outlineFactory);

    addAutoReleasedObject(new FindInFiles);
    addAutoReleasedObject(new FindInCurrentFile);
    addAutoReleasedObject(new FindInOpenFiles);

    return true;
}

void TextEditorPlugin::extensionsInitialized()
{
    m_outlineFactory->setWidgetFactories(ExtensionSystem::PluginManager::getObjects<TextEditor::IOutlineWidgetFactory>());

    connect(m_settings, &TextEditorSettings::fontSettingsChanged,
            this, &TextEditorPlugin::updateSearchResultsFont);

    updateSearchResultsFont(m_settings->fontSettings());

    connect(m_settings->codeStyle(), &ICodeStylePreferences::currentTabSettingsChanged,
            this, &TextEditorPlugin::updateSearchResultsTabWidth);

    updateSearchResultsTabWidth(m_settings->codeStyle()->currentTabSettings());

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


    connect(ExternalToolManager::instance(), &ExternalToolManager::replaceSelectionRequested,
            this, &TextEditorPlugin::updateCurrentSelection);
}

LineNumberFilter *TextEditorPlugin::lineNumberFilter()
{
    return m_instance->m_lineNumberFilter;
}

void TextEditorPlugin::updateSearchResultsFont(const FontSettings &settings)
{
    if (auto window = SearchResultWindow::instance()) {
        window->setTextEditorFont(QFont(settings.family(), settings.fontSize() * settings.fontZoom() / 100),
                                  settings.formatFor(C_TEXT).foreground(),
                                  settings.formatFor(C_TEXT).background(),
                                  settings.formatFor(C_SEARCH_RESULT).foreground(),
                                  settings.formatFor(C_SEARCH_RESULT).background());
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

} // namespace Internal
} // namespace TextEditor
