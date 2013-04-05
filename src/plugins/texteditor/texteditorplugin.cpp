/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "plaintexteditor.h"
#include "manager.h"
#include "outlinefactory.h"
#include "snippets/plaintextsnippetprovider.h"
#include "basetextmarkregistry.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/externaltoolmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <utils/qtcassert.h>

#include <QtPlugin>
#include <QShortcut>
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

TextEditorPlugin *TextEditorPlugin::m_instance = 0;

TextEditorPlugin::TextEditorPlugin()
  : m_settings(0),
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

static const char wizardCategoryC[] = "U.General";

static inline QString wizardDisplayCategory()
{
    return TextEditorPlugin::tr("General");
}

// A wizard that quickly creates a scratch buffer
// based on a temporary file without prompting for a path.
class ScratchFileWizard : public Core::IWizard
{
    Q_OBJECT

public:
    virtual WizardKind kind() const { return FileWizard; }
    virtual QIcon icon() const { return QIcon(); }
    virtual QString description() const
        { return TextEditorPlugin::tr("Creates a scratch buffer using a temporary file."); }
    virtual QString displayName() const
        { return TextEditorPlugin::tr("Scratch Buffer"); }
    virtual QString id() const
        { return QLatin1String("Z.ScratchFile"); }
    virtual QString category() const
        { return QLatin1String(wizardCategoryC); }
    virtual QString displayCategory() const
        {  return wizardDisplayCategory(); }
    virtual QString descriptionImage() const
        { return QString(); }
    virtual Core::FeatureSet requiredFeatures() const
        { return Core::FeatureSet(); }
    virtual WizardFlags flags() const
        { return Core::IWizard::PlatformIndependent; }
    virtual void runWizard(const QString &, QWidget *, const QString &, const QVariantMap &)
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
    Core::EditorManager::openEditor(file.fileName(), Core::Id(), Core::EditorManager::ModeSwitch);
}

// ExtensionSystem::PluginInterface
bool TextEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/texteditor/TextEditor.mimetypes.xml"), errorMessage))
        return false;

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Creates a text file. The default file extension is <tt>.txt</tt>. "
                                       "You can specify a different extension as part of the filename."));
    wizardParameters.setDisplayName(tr("Text File"));
    wizardParameters.setCategory(QLatin1String(wizardCategoryC));
    wizardParameters.setDisplayCategory(wizardDisplayCategory());
    wizardParameters.setFlags(Core::IWizard::PlatformIndependent);
    TextFileWizard *wizard = new TextFileWizard(QLatin1String(Constants::C_TEXTEDITOR_MIMETYPE_TEXT),
                                                QLatin1String("text$"),
                                                wizardParameters);
    // Add text file wizard
    addAutoReleasedObject(wizard);
    ScratchFileWizard *scratchFile = new ScratchFileWizard;
    addAutoReleasedObject(scratchFile);

    m_settings = new TextEditorSettings(this);

    // Add plain text editor factory
    m_editorFactory = new PlainTextEditorFactory;
    addAutoReleasedObject(m_editorFactory);

    // Goto line functionality for quick open
    m_lineNumberFilter = new LineNumberFilter;
    addAutoReleasedObject(m_lineNumberFilter);

    Core::Context context(TextEditor::Constants::C_TEXTEDITOR);

    // Add shortcut for invoking automatic completion
    QShortcut *completionShortcut = new QShortcut(Core::ICore::mainWindow());
    completionShortcut->setWhatsThis(tr("Triggers a completion in this scope"));
    // Make sure the shortcut still works when the completion widget is active
    completionShortcut->setContext(Qt::ApplicationShortcut);
    Core::Command *command = Core::ActionManager::registerShortcut(completionShortcut, Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Space") : tr("Ctrl+Space")));
    connect(completionShortcut, SIGNAL(activated()), this, SLOT(invokeCompletion()));

    // Add shortcut for invoking quick fix options
    QShortcut *quickFixShortcut = new QShortcut(Core::ICore::mainWindow());
    quickFixShortcut->setWhatsThis(tr("Triggers a quick fix in this scope"));
    // Make sure the shortcut still works when the quick fix widget is active
    quickFixShortcut->setContext(Qt::ApplicationShortcut);
    Core::Command *quickFixCommand = Core::ActionManager::registerShortcut(quickFixShortcut, Constants::QUICKFIX_THIS, context);
    quickFixCommand->setDefaultKeySequence(QKeySequence(tr("Alt+Return")));
    connect(quickFixShortcut, SIGNAL(activated()), this, SLOT(invokeQuickFix()));

    // Add shortcut for create a scratch buffer
    QShortcut *scratchBufferShortcut = new QShortcut(Core::ICore::mainWindow());
    scratchBufferShortcut->setWhatsThis(tr("Creates a scratch buffer using a temporary file."));
    scratchBufferShortcut->setContext(Qt::ApplicationShortcut);
    Core::ActionManager::registerShortcut(scratchBufferShortcut, Constants::CREATE_SCRATCH_BUFFER, context);
    connect(scratchBufferShortcut, SIGNAL(activated()), scratchFile, SLOT(createFile()));

    // Generic highlighter.
    connect(Core::ICore::instance(), SIGNAL(coreOpened()),
            Manager::instance(), SLOT(registerMimeTypes()));

    // Add text snippet provider.
    addAutoReleasedObject(new PlainTextSnippetProvider);

    m_outlineFactory = new OutlineFactory;
    addAutoReleasedObject(m_outlineFactory);

    // We have to initialize the actions because other plugins that
    // depend upon the texteditorplugin expect that actions will be
    // registered in the action manager at plugin initialization time.
    m_editorFactory->actionHandler()->initializeActions();

    m_baseTextMarkRegistry = new BaseTextMarkRegistry(this);

    return true;
}

void TextEditorPlugin::extensionsInitialized()
{
    m_searchResultWindow = Find::SearchResultWindow::instance();

    m_outlineFactory->setWidgetFactories(ExtensionSystem::PluginManager::getObjects<TextEditor::IOutlineWidgetFactory>());

    connect(m_settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(updateSearchResultsFont(TextEditor::FontSettings)));

    updateSearchResultsFont(m_settings->fontSettings());

    addAutoReleasedObject(new FindInFiles);
    addAutoReleasedObject(new FindInCurrentFile);
    addAutoReleasedObject(new FindInOpenFiles);

    Core::VariableManager::registerVariable(kCurrentDocumentSelection,
        tr("Selected text within the current document."));
    Core::VariableManager::registerVariable(kCurrentDocumentRow,
        tr("Line number of the text cursor position in current document (starts with 1)."));
    Core::VariableManager::registerVariable(kCurrentDocumentColumn,
        tr("Column number of the text cursor position in current document (starts with 0)."));
    Core::VariableManager::registerVariable(kCurrentDocumentRowCount,
        tr("Number of lines visible in current document."));
    Core::VariableManager::registerVariable(kCurrentDocumentColumnCount,
        tr("Number of columns visible in current document."));
    Core::VariableManager::registerVariable(kCurrentDocumentFontSize,
        tr("Current document's font size in points."));
    connect(Core::VariableManager::instance(), SIGNAL(variableUpdateRequested(QByteArray)),
            this, SLOT(updateVariable(QByteArray)));
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

void TextEditorPlugin::updateVariable(const QByteArray &variable)
{
    static QSet<QByteArray> variables = QSet<QByteArray>()
            << kCurrentDocumentSelection
            << kCurrentDocumentRow
            << kCurrentDocumentColumn
            << kCurrentDocumentRowCount
            << kCurrentDocumentColumnCount
            << kCurrentDocumentFontSize;
    if (variables.contains(variable)) {
        QString value;
        Core::IEditor *iface = Core::EditorManager::currentEditor();
        ITextEditor *editor = qobject_cast<ITextEditor *>(iface);
        if (editor) {
            if (variable == kCurrentDocumentSelection) {
                value = editor->selectedText();
                value.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
            } else if (variable == kCurrentDocumentRow) {
                value = QString::number(editor->currentLine());
            } else if (variable == kCurrentDocumentColumn) {
                value = QString::number(editor->currentColumn());
            } else if (variable == kCurrentDocumentRowCount) {
                value = QString::number(editor->rowCount());
            } else if (variable == kCurrentDocumentColumnCount) {
                value = QString::number(editor->columnCount());
            } else if (variable == kCurrentDocumentFontSize) {
                value = QString::number(editor->widget()->font().pointSize());
            }
        }
        Core::VariableManager::insert(variable, value);
    }
}

void TextEditorPlugin::updateCurrentSelection(const QString &text)
{
    Core::IEditor *iface = Core::EditorManager::currentEditor();
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

#include "texteditorplugin.moc"
