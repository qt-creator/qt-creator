/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "texteditorplugin.h"

#include "completionsupport.h"
#include "findinfiles.h"
#include "findincurrentfile.h"
#include "fontsettings.h"
#include "linenumberfilter.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "textfilewizard.h"
#include "plaintexteditorfactory.h"
#include "plaintexteditor.h"
#include "storagesettings.h"
#include "manager.h"
#include "outlinefactory.h"
#include "snippets/plaintextsnippetprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/externaltool.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <find/searchresultwindow.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtGui/QMainWindow>
#include <QtGui/QShortcut>

using namespace TextEditor;
using namespace TextEditor::Internal;

static const char * const kCurrentDocumentSelection = "CurrentDocument:Selection";
static const char * const kCurrentDocumentRow = "CurrentDocument:Row";
static const char * const kCurrentDocumentColumn = "CurrentDocument:Column";
static const char * const kCurrentDocumentRowCount = "CurrentDocument:RowCount";
static const char * const kCurrentDocumentColumnCount = "CurrentDocument:ColumnCount";
static const char * const kCurrentDocumentFontSize = "CurrentDocument:FontSize";

TextEditorPlugin *TextEditorPlugin::m_instance = 0;

TextEditorPlugin::TextEditorPlugin()
  : m_settings(0),
    m_wizard(0),
    m_editorFactory(0),
    m_lineNumberFilter(0),
    m_searchResultWindow(0)
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

    if (!Core::ICore::instance()->mimeDatabase()->addMimeTypes(QLatin1String(":/texteditor/TextEditor.mimetypes.xml"), errorMessage))
        return false;

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Creates a text file. The default file extension is <tt>.txt</tt>. "
                                       "You can specify a different extension as part of the filename."));
    wizardParameters.setDisplayName(tr("Text File"));
    wizardParameters.setCategory(QLatin1String("U.General"));
    wizardParameters.setDisplayCategory(tr("General"));
    m_wizard = new TextFileWizard(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT),
                                  QLatin1String("text$"),
                                  wizardParameters);
    // Add text file wizard
    addAutoReleasedObject(m_wizard);

    m_settings = new TextEditorSettings(this);

    // Add plain text editor factory
    m_editorFactory = new PlainTextEditorFactory;
    addAutoReleasedObject(m_editorFactory);

    // Goto line functionality for quick open
    Core::ICore *core = Core::ICore::instance();
    m_lineNumberFilter = new LineNumberFilter;
    addAutoReleasedObject(m_lineNumberFilter);

    Core::Context context(TextEditor::Constants::C_TEXTEDITOR);
    Core::ActionManager *am = core->actionManager();

    // Add shortcut for invoking automatic completion
    QShortcut *completionShortcut = new QShortcut(core->mainWindow());
    completionShortcut->setWhatsThis(tr("Triggers a completion in this scope"));
    // Make sure the shortcut still works when the completion widget is active
    completionShortcut->setContext(Qt::ApplicationShortcut);
    Core::Command *command = am->registerShortcut(completionShortcut, Constants::COMPLETE_THIS, context);
#ifndef Q_WS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Space")));
#else
    command->setDefaultKeySequence(QKeySequence(tr("Meta+Space")));
#endif
    connect(completionShortcut, SIGNAL(activated()), this, SLOT(invokeCompletion()));

    // Add shortcut for invoking quick fix options
    QShortcut *quickFixShortcut = new QShortcut(core->mainWindow());
    quickFixShortcut->setWhatsThis(tr("Triggers a quick fix in this scope"));
    // Make sure the shortcut still works when the quick fix widget is active
    quickFixShortcut->setContext(Qt::ApplicationShortcut);
    Core::Command *quickFixCommand = am->registerShortcut(quickFixShortcut, Constants::QUICKFIX_THIS, context);
    quickFixCommand->setDefaultKeySequence(QKeySequence(tr("Alt+Return")));
    connect(quickFixShortcut, SIGNAL(activated()), this, SLOT(invokeQuickFix()));

    // Generic highlighter.
    connect(Core::ICore::instance(), SIGNAL(coreOpened()),
            Manager::instance(), SLOT(registerMimeTypes()));

    // Add text snippet provider.
    addAutoReleasedObject(new PlainTextSnippetProvider);

    m_outlineFactory = new OutlineFactory;
    addAutoReleasedObject(m_outlineFactory);

    return true;
}

void TextEditorPlugin::extensionsInitialized()
{
    m_editorFactory->actionHandler()->initializeActions();

    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();

    m_searchResultWindow = Find::SearchResultWindow::instance();

    m_outlineFactory->setWidgetFactories(pluginManager->getObjects<TextEditor::IOutlineWidgetFactory>());

    connect(m_settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(updateSearchResultsFont(TextEditor::FontSettings)));

    updateSearchResultsFont(m_settings->fontSettings());

    addAutoReleasedObject(new FindInFiles(Find::SearchResultWindow::instance()));
    addAutoReleasedObject(new FindInCurrentFile(Find::SearchResultWindow::instance()));

    Core::VariableManager *vm = Core::VariableManager::instance();
    vm->registerVariable(QLatin1String(kCurrentDocumentSelection),
        tr("Selected text within the current document."));
    vm->registerVariable(QLatin1String(kCurrentDocumentRow),
        tr("Line number of the text cursor position in current document (starts with 1)."));
    vm->registerVariable(QLatin1String(kCurrentDocumentColumn),
        tr("Column number of the text cursor position in current document (starts with 0)."));
    vm->registerVariable(QLatin1String(kCurrentDocumentRowCount),
        tr("Number of lines visible in current document."));
    vm->registerVariable(QLatin1String(kCurrentDocumentColumnCount),
        tr("Number of columns visible in current document."));
    vm->registerVariable(QLatin1String(kCurrentDocumentFontSize),
        tr("Current document's font size in points."));
    connect(vm, SIGNAL(variableUpdateRequested(QString)),
            this, SLOT(updateVariable(QString)));
    connect(Core::ExternalToolManager::instance(), SIGNAL(replaceSelectionRequested(QString)),
            this, SLOT(updateCurrentSelection(QString)));
}

void TextEditorPlugin::initializeEditor(PlainTextEditorWidget *editor)
{
    // common actions
    m_editorFactory->actionHandler()->setupActions(editor);

    TextEditorSettings::instance()->initializeEditor(editor);
}

void TextEditorPlugin::invokeCompletion()
{
    Core::IEditor *iface = Core::EditorManager::instance()->currentEditor();
    ITextEditor *editor = qobject_cast<ITextEditor *>(iface);
    if (editor)
        CompletionSupport::instance()->complete(editor, SemanticCompletion, true);
}

void TextEditorPlugin::invokeQuickFix()
{
    Core::IEditor *iface = Core::EditorManager::instance()->currentEditor();
    ITextEditor *editor = qobject_cast<ITextEditor *>(iface);
    if (editor)
        CompletionSupport::instance()->complete(editor, QuickFixCompletion, true);
}

void TextEditorPlugin::updateSearchResultsFont(const FontSettings &settings)
{
    if (m_searchResultWindow)
        m_searchResultWindow->setTextEditorFont(QFont(settings.family(),
                                                      settings.fontSize() * settings.fontZoom() / 100));
}

void TextEditorPlugin::updateVariable(const QString &variable)
{
    static QSet<QString> variables = QSet<QString>()
            << QString::fromLatin1(kCurrentDocumentSelection)
            << QString::fromLatin1(kCurrentDocumentRow)
            << QString::fromLatin1(kCurrentDocumentColumn)
            << QString::fromLatin1(kCurrentDocumentRowCount)
            << QString::fromLatin1(kCurrentDocumentColumnCount)
            << QString::fromLatin1(kCurrentDocumentFontSize);
    if (variables.contains(variable)) {
        QString value;
        Core::IEditor *iface = Core::EditorManager::instance()->currentEditor();
        ITextEditor *editor = qobject_cast<ITextEditor *>(iface);
        if (editor) {
            if (variable == QLatin1String(kCurrentDocumentSelection)) {
                value = editor->selectedText();
                value.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
            } else if (variable == QLatin1String(kCurrentDocumentRow)) {
                value = QString::number(editor->currentLine());
            } else if (variable == QLatin1String(kCurrentDocumentColumn)) {
                value = QString::number(editor->currentColumn());
            } else if (variable == QLatin1String(kCurrentDocumentRowCount)) {
                value = QString::number(editor->rowCount());
            } else if (variable == QLatin1String(kCurrentDocumentColumnCount)) {
                value = QString::number(editor->columnCount());
            } else if (variable == QLatin1String(kCurrentDocumentFontSize)) {
                value = QString::number(editor->widget()->font().pointSize());
            }
        }
        Core::VariableManager::instance()->insert(variable, value);
    }
}

void TextEditorPlugin::updateCurrentSelection(const QString &text)
{
    Core::IEditor *iface = Core::EditorManager::instance()->currentEditor();
    ITextEditor *editor = qobject_cast<ITextEditor *>(iface);
    if (editor) {
        int pos = editor->position();
        int anchor = editor->position(ITextEditor::Anchor);
        if (anchor < 0) // no selection
            anchor = pos;
        int selectionLength = anchor-pos;
        if (selectionLength < 0)
            selectionLength = -selectionLength;
        int start = qMin(pos, anchor);
        editor->setCursorPosition(start);
        editor->replace(selectionLength, text);
    }
}

Q_EXPORT_PLUGIN(TextEditorPlugin)
