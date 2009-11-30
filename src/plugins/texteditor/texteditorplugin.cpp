/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "texteditorplugin.h"

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

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <find/searchresultwindow.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtGui/QMainWindow>
#include <QtGui/QShortcut>

using namespace TextEditor;
using namespace TextEditor::Internal;

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
    wizardParameters.setDescription(tr("Creates a text file (.txt)."));
    wizardParameters.setName(tr("Text File"));
    wizardParameters.setCategory(QLatin1String("O.General"));
    wizardParameters.setTrCategory(tr("General"));
    m_wizard = new TextFileWizard(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT),
                                  QLatin1String(Core::Constants::K_DEFAULT_TEXT_EDITOR),
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

    int contextId = core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
    QList<int> context = QList<int>() << contextId;
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

    return true;
}

void TextEditorPlugin::extensionsInitialized()
{
    m_editorFactory->actionHandler()->initializeActions();

    m_searchResultWindow = ExtensionSystem::PluginManager::instance()->getObject<Find::SearchResultWindow>();

    connect(m_settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(updateSearchResultsFont(TextEditor::FontSettings)));

    updateSearchResultsFont(m_settings->fontSettings());

    addAutoReleasedObject(new FindInFiles(
        ExtensionSystem::PluginManager::instance()->getObject<Find::SearchResultWindow>()));
    addAutoReleasedObject(new FindInCurrentFile(
        ExtensionSystem::PluginManager::instance()->getObject<Find::SearchResultWindow>()));
}

void TextEditorPlugin::initializeEditor(PlainTextEditor *editor)
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
        editor->triggerCompletions();
}

void TextEditorPlugin::invokeQuickFix()
{
    Core::IEditor *iface = Core::EditorManager::instance()->currentEditor();
    ITextEditor *editor = qobject_cast<ITextEditor *>(iface);
    if (editor)
        editor->triggerQuickFix();
}

void TextEditorPlugin::updateSearchResultsFont(const FontSettings &settings)
{
    if (m_searchResultWindow)
        m_searchResultWindow->setTextEditorFont(QFont(settings.family(),
                                                      settings.fontSize() * settings.fontZoom() / 100));
}

Q_EXPORT_PLUGIN(TextEditorPlugin)
