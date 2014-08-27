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

#include "texteditorplugin.h"

#include "findinfiles.h"
#include "findincurrentfile.h"
#include "findinopenfiles.h"
#include "fontsettings.h"
#include "linenumberfilter.h"
#include "texteditorsettings.h"
#include "textfilewizard.h"
#include "plaintexteditorfactory.h"
#include "basetexteditor.h"
#include "outlinefactory.h"
#include "snippets/plaintextsnippetprovider.h"
#include "textmarkregistry.h"
#include <texteditor/generichighlighter/manager.h>

#include <coreplugin/icore.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/externaltoolmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <utils/qtcassert.h>

#include <QtPlugin>
#include <QAction>
#include <QDir>
#include <QTemporaryFile>

using namespace TextEditor;
using namespace TextEditor::Internal;

static const char kCurrentDocumentSelection[] = "CurrentDocument:Selection";
static const char kCurrentDocumentRow[] = "CurrentDocument:Row";
static const char kCurrentDocumentColumn[] = "CurrentDocument:Column";
static const char kCurrentDocumentRowCount[] = "CurrentDocument:RowCount";
static const char kCurrentDocumentColumnCount[] = "CurrentDocument:ColumnCount";
static const char kCurrentDocumentFontSize[] = "CurrentDocument:FontSize";

static TextEditorPlugin *m_instance = 0;

TextEditorPlugin::TextEditorPlugin()
  : m_settings(0),
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

static const char wizardCategoryC[] = "U.General";

static inline QString wizardDisplayCategory()
{
    return TextEditorPlugin::tr("General");
}

// A wizard that quickly creates a scratch buffer
// based on a temporary file without prompting for a path.
class ScratchFileWizard : public Core::IWizardFactory
{
    Q_OBJECT

public:
    ScratchFileWizard()
    {
        setWizardKind(FileWizard);
        setDescription(TextEditorPlugin::tr("Creates a scratch buffer using a temporary file."));
        setDisplayName(TextEditorPlugin::tr("Scratch Buffer"));
        setId(QLatin1String("Z.ScratchFile"));
        setCategory(QLatin1String(wizardCategoryC));
        setDisplayCategory(wizardDisplayCategory());
        setFlags(Core::IWizardFactory::PlatformIndependent);
    }

    void runWizard(const QString &, QWidget *, const QString &, const QVariantMap &)
    { createFile(); }

public Q_SLOTS:
    virtual void createFile();
};

void ScratchFileWizard::createFile()
{
    QString tempPattern = QDir::tempPath();
    if (!tempPattern.endsWith(QLatin1Char('/')))
        tempPattern += QLatin1Char('/');
    tempPattern += QLatin1String("scratchXXXXXX.txt");
    QTemporaryFile file(tempPattern);
    file.setAutoRemove(false);
    QTC_ASSERT(file.open(), return; );
    file.close();
    Core::EditorManager::openEditor(file.fileName());
}

// ExtensionSystem::PluginInterface
bool TextEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    if (!Core::MimeDatabase::addMimeTypes(QLatin1String(":/texteditor/TextEditor.mimetypes.xml"), errorMessage))
        return false;

    TextFileWizard *wizard = new TextFileWizard(QLatin1String(Constants::C_TEXTEDITOR_MIMETYPE_TEXT),
                                                QLatin1String("text$"));
    wizard->setWizardKind(Core::IWizardFactory::FileWizard);
    wizard->setDescription(tr("Creates a text file. The default file extension is <tt>.txt</tt>. "
                                       "You can specify a different extension as part of the filename."));
    wizard->setDisplayName(tr("Text File"));
    wizard->setCategory(QLatin1String(wizardCategoryC));
    wizard->setDisplayCategory(wizardDisplayCategory());
    wizard->setFlags(Core::IWizardFactory::PlatformIndependent);

    // Add text file wizard
    addAutoReleasedObject(wizard);
    ScratchFileWizard *scratchFile = new ScratchFileWizard;
    addAutoReleasedObject(scratchFile);

    m_settings = new TextEditorSettings(this);

    // Add plain text editor factory
    addAutoReleasedObject(new PlainTextEditorFactory);

    // Goto line functionality for quick open
    m_lineNumberFilter = new LineNumberFilter;
    addAutoReleasedObject(m_lineNumberFilter);

    Core::Context context(TextEditor::Constants::C_TEXTEDITOR);

    // Add shortcut for invoking automatic completion
    QAction *completionAction = new QAction(tr("Trigger Completion"), this);
    Core::Command *command = Core::ActionManager::registerAction(completionAction, Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Space") : tr("Ctrl+Space")));
    connect(completionAction, SIGNAL(triggered()), this, SLOT(invokeCompletion()));

    // Add shortcut for invoking quick fix options
    QAction *quickFixAction = new QAction(tr("Trigger Refactoring Action"), this);
    Core::Command *quickFixCommand = Core::ActionManager::registerAction(quickFixAction, Constants::QUICKFIX_THIS, context);
    quickFixCommand->setDefaultKeySequence(QKeySequence(tr("Alt+Return")));
    connect(quickFixAction, SIGNAL(triggered()), this, SLOT(invokeQuickFix()));

    // Add shortcut for create a scratch buffer
    QAction *scratchBufferAction = new QAction(tr("Create Scratch Buffer Using a Temporary File"), this);
    Core::ActionManager::registerAction(scratchBufferAction, Constants::CREATE_SCRATCH_BUFFER, context);
    connect(scratchBufferAction, SIGNAL(triggered()), scratchFile, SLOT(createFile()));

    // Generic highlighter.
    connect(Core::ICore::instance(), SIGNAL(coreOpened()),
            Manager::instance(), SLOT(registerMimeTypes()));

    // Add text snippet provider.
    addAutoReleasedObject(new PlainTextSnippetProvider);

    m_outlineFactory = new OutlineFactory;
    addAutoReleasedObject(m_outlineFactory);

    m_baseTextMarkRegistry = new TextMarkRegistry(this);

    return true;
}

void TextEditorPlugin::extensionsInitialized()
{
    m_searchResultWindow = Core::SearchResultWindow::instance();

    m_outlineFactory->setWidgetFactories(ExtensionSystem::PluginManager::getObjects<TextEditor::IOutlineWidgetFactory>());

    connect(m_settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(updateSearchResultsFont(TextEditor::FontSettings)));

    updateSearchResultsFont(m_settings->fontSettings());

    addAutoReleasedObject(new FindInFiles);
    addAutoReleasedObject(new FindInCurrentFile);
    addAutoReleasedObject(new FindInOpenFiles);

    Core::VariableManager::registerVariable(kCurrentDocumentSelection,
        tr("Selected text within the current document."),
        []() -> QString {
            QString value;
            if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor()) {
                value = editor->selectedText();
                value.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
            }
            return value;
        });

    Core::VariableManager::registerIntVariable(kCurrentDocumentRow,
        tr("Line number of the text cursor position in current document (starts with 1)."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->currentLine() : 0;
        });

    Core::VariableManager::registerIntVariable(kCurrentDocumentColumn,
        tr("Column number of the text cursor position in current document (starts with 0)."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->currentColumn() : 0;
        });

    Core::VariableManager::registerIntVariable(kCurrentDocumentRowCount,
        tr("Number of lines visible in current document."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->rowCount() : 0;
        });

    Core::VariableManager::registerIntVariable(kCurrentDocumentColumnCount,
        tr("Number of columns visible in current document."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->columnCount() : 0;
        });

    Core::VariableManager::registerIntVariable(kCurrentDocumentFontSize,
        tr("Current document's font size in points."),
        []() -> int {
            BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
            return editor ? editor->widget()->font().pointSize() : 0;
        });


    connect(Core::ExternalToolManager::instance(), SIGNAL(replaceSelectionRequested(QString)),
            this, SLOT(updateCurrentSelection(QString)));
}

LineNumberFilter *TextEditorPlugin::lineNumberFilter()
{
    return m_instance->m_lineNumberFilter;
}

TextMarkRegistry *TextEditorPlugin::baseTextMarkRegistry()
{
    return m_instance->m_baseTextMarkRegistry;
}

void TextEditorPlugin::invokeCompletion()
{
    Core::IEditor *iface = Core::EditorManager::currentEditor();
    if (BaseTextEditorWidget *w = qobject_cast<BaseTextEditorWidget *>(iface->widget()))
        w->invokeAssist(Completion);
}

void TextEditorPlugin::invokeQuickFix()
{
    Core::IEditor *iface = Core::EditorManager::currentEditor();
    if (BaseTextEditorWidget *w = qobject_cast<BaseTextEditorWidget *>(iface->widget()))
        w->invokeAssist(QuickFix);
}

void TextEditorPlugin::updateSearchResultsFont(const FontSettings &settings)
{
    if (m_searchResultWindow) {
        m_searchResultWindow->setTextEditorFont(QFont(settings.family(),
                                                      settings.fontSize() * settings.fontZoom() / 100),
                                                settings.formatFor(TextEditor::C_TEXT).foreground(),
                                                settings.formatFor(TextEditor::C_TEXT).background(),
                                                settings.formatFor(TextEditor::C_SEARCH_RESULT).foreground(),
                                                settings.formatFor(TextEditor::C_SEARCH_RESULT).background());
    }
}

void TextEditorPlugin::updateCurrentSelection(const QString &text)
{
    if (BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(Core::EditorManager::currentEditor())) {
        const int pos = editor->position();
        int anchor = editor->position(BaseTextEditor::Anchor);
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

#include "texteditorplugin.moc"
