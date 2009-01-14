/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qtscripteditorplugin.h"

#include "qscripthighlighter.h"
#include "qtscripteditor.h"
#include "qtscripteditorconstants.h"
#include "qtscripteditorfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <utils/qtcassert.h>

#include <QtCore/qplugin.h>
#include <QtCore/QDebug>
#include <QtGui/QAction>

using namespace QtScriptEditor::Internal;
using namespace QtScriptEditor::Constants;

QtScriptEditorPlugin *QtScriptEditorPlugin::m_instance = 0;

QtScriptEditorPlugin::QtScriptEditorPlugin() :
    m_wizard(0),
    m_editor(0)
{
    m_instance = this;
}

QtScriptEditorPlugin::~QtScriptEditorPlugin()
{
    removeObject(m_editor);
    removeObject(m_wizard);
    m_instance = 0;
}

bool QtScriptEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    typedef SharedTools::QScriptHighlighter QScriptHighlighter;

    Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/qtscripteditor/QtScriptEditor.mimetypes.xml"), error_message))
        return false;
    m_scriptcontext << core->uniqueIDManager()->uniqueIdentifier(QtScriptEditor::Constants::C_QTSCRIPTEDITOR);
    m_context = m_scriptcontext;
    m_context << core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);

    registerActions(core);

    m_editor = new QtScriptEditorFactory(core, m_context, this);
    addObject(m_editor);

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Qt Script file"));
    wizardParameters.setName(tr("Qt Script file"));
    wizardParameters.setCategory(QLatin1String("Qt"));
    wizardParameters.setTrCategory(tr("Qt"));
    m_wizard = new TextEditor::TextFileWizard(QLatin1String(QtScriptEditor::Constants::C_QTSCRIPTEDITOR_MIMETYPE),
                                              QLatin1String(QtScriptEditor::Constants::C_QTSCRIPTEDITOR),
                                              QLatin1String("qtscript$"),
                                              wizardParameters, core, this);
    addObject(m_wizard);

    error_message->clear();
    return true;
}

void QtScriptEditorPlugin::extensionsInitialized()
{
}

void QtScriptEditorPlugin::initializeEditor(QtScriptEditor::Internal::ScriptEditor *editor)
{
    QTC_ASSERT(m_instance, /**/);

    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            editor, SLOT(setFontSettings(TextEditor::FontSettings)));
    connect(settings, SIGNAL(tabSettingsChanged(TextEditor::TabSettings)),
            editor, SLOT(setTabSettings(TextEditor::TabSettings)));
    connect(settings, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            editor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(settings, SIGNAL(displaySettingsChanged(TextEditor::DisplaySettings)),
            editor, SLOT(setDisplaySettings(TextEditor::DisplaySettings)));

    // tab settings rely on font settings
    editor->setFontSettings(settings->fontSettings());
    editor->setTabSettings(settings->tabSettings());
    editor->setStorageSettings(settings->storageSettings());
    editor->setDisplaySettings(settings->displaySettings());
}

void QtScriptEditorPlugin::registerActions(Core::ICore *core)
{
    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *mcontext = am->createMenu(QtScriptEditor::Constants::M_CONTEXT);

    QAction *action = new QAction(this);
    action->setSeparator(true);
    Core::ICommand *cmd = am->registerAction(action, QtScriptEditor::Constants::RUN_SEP, m_scriptcontext);
    mcontext->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    action = new QAction(tr("Run"), this);
    cmd = am->registerAction(action, QtScriptEditor::Constants::RUN, m_scriptcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+R")));
    mcontext->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
}

Q_EXPORT_PLUGIN(QtScriptEditorPlugin)
