/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "texteditorplugin.h"

#include "findinfiles.h"
#include "fontsettings.h"
#include "linenumberfilter.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "textfilewizard.h"
#include "plaintexteditorfactory.h"
#include "plaintexteditor.h"
#include "storagesettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/icommand.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <utils/qtcassert.h>

#include <QtCore/qplugin.h>
#include <QtGui/QShortcut>
#include <QtGui/QMainWindow>

using namespace TextEditor;
using namespace TextEditor::Internal;

TextEditorPlugin *TextEditorPlugin::m_instance = 0;

TextEditorPlugin::TextEditorPlugin() :
    m_core(0),
    m_settings(0),
    m_wizard(0),
    m_editorFactory(0),
    m_lineNumberFilter(0)
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

Core::ICore *TextEditorPlugin::core()
{
    return m_instance->m_core;
}

//ExtensionSystem::PluginInterface
bool TextEditorPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();

    if (!m_core->mimeDatabase()->addMimeTypes(QLatin1String(":/texteditor/TextEditor.mimetypes.xml"), errorMessage))
        return false;

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("This creates a new text file (.txt)"));
    wizardParameters.setName(tr("Text File"));
    wizardParameters.setCategory(QLatin1String("General"));
    wizardParameters.setTrCategory(tr("General"));
    m_wizard = new TextFileWizard(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT),
                                  QLatin1String(Core::Constants::K_DEFAULT_TEXT_EDITOR),
                                  QLatin1String("text$"),
                                  wizardParameters, m_core);
    // Add text file wizard
    addAutoReleasedObject(m_wizard);


    m_settings = new TextEditorSettings(this, this);

    // Add plain text editor factory
    m_editorFactory = new PlainTextEditorFactory;
    addAutoReleasedObject(m_editorFactory);

    // Goto line functionality for quick open
    m_lineNumberFilter = new LineNumberFilter(m_core->editorManager());
    addAutoReleasedObject(m_lineNumberFilter);

    int contextId = m_core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
    QList<int> context = QList<int>() << contextId;
    Core::ActionManager *am = m_core->actionManager();

    // Add shortcut for invoking automatic completion
    QShortcut *completionShortcut = new QShortcut(m_core->mainWindow());
    completionShortcut->setWhatsThis(tr("Triggers a completion in this scope"));
    // Make sure the shortcut still works when the completion widget is active
    completionShortcut->setContext(Qt::ApplicationShortcut);
    Core::ICommand *command = am->registerShortcut(completionShortcut, Constants::COMPLETE_THIS, context);
#ifndef Q_OS_MAC
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Space")));
#else
    command->setDefaultKeySequence(QKeySequence(tr("Meta+Space")));
#endif
    connect(completionShortcut, SIGNAL(activated()), this, SLOT(invokeCompletion()));

    addAutoReleasedObject(new FindInFiles(m_core, m_core->pluginManager()->getObject<Find::SearchResultWindow>()));

    return true;
}

void TextEditorPlugin::extensionsInitialized()
{
    m_editorFactory->actionHandler()->initializeActions();
}

void TextEditorPlugin::initializeEditor(TextEditor::PlainTextEditor *editor)
{
    // common actions
    m_editorFactory->actionHandler()->setupActions(editor);

    // settings
    connect(m_settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            editor, SLOT(setFontSettings(TextEditor::FontSettings)));
    connect(m_settings, SIGNAL(tabSettingsChanged(TextEditor::TabSettings)),
            editor, SLOT(setTabSettings(TextEditor::TabSettings)));
    connect(m_settings, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            editor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(m_settings, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            editor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));

    // tab settings rely on font settings
    editor->setFontSettings(m_settings->fontSettings());
    editor->setTabSettings(m_settings->tabSettings());
    editor->setStorageSettings(m_settings->storageSettings());
    editor->setDisplaySettings(m_settings->displaySettings());
}

void TextEditorPlugin::invokeCompletion()
{
    if (!m_core)
        return;

    Core::IEditor *iface = m_core->editorManager()->currentEditor();
    ITextEditor *editor = qobject_cast<ITextEditor *>(iface);
    if (editor)
        editor->triggerCompletions();
}


Q_EXPORT_PLUGIN(TextEditorPlugin)
