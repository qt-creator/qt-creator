/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "exception.h"
#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"
#include "pluginmanager.h"
#include "designmodewidget.h"
#include "settingspage.h"
#include "designmodecontext.h"

#include <coreplugin/designmode.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <coreplugin/modemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <integrationcore.h>

#include <QtGui/QAction>

#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/qplugin.h>
#include <QtCore/QDebug>

namespace QmlDesigner {
namespace Internal {

BauhausPlugin *BauhausPlugin::m_pluginInstance = 0;

BauhausPlugin::BauhausPlugin() :
    m_designerCore(0),
    m_designMode(0),
    m_isActive(false),
    m_revertToSavedAction(new QAction(this)),
    m_saveAction(new QAction(this)),
    m_saveAsAction(new QAction(this)),
    m_closeCurrentEditorAction(new QAction(this)),
    m_closeAllEditorsAction(new QAction(this)),
    m_closeOtherEditorsAction(new QAction(this))
{

    // Exceptions should never ever assert: they are handled in a number of
    // places where it is actually VALID AND EXPECTED BEHAVIOUR to get an
    // exception.
    // If you still want to see exactly where the exception originally
    // occurred, then you have various ways to do this:
    //  1. set a breakpoint on the constructor of the exception
    //  2. in gdb: "catch throw" or "catch throw Exception"
    //  3. set a breakpoint on __raise_exception()
    // And with gdb, you can even do this from your ~/.gdbinit file.
    Exception::setShouldAssert(false);
}

BauhausPlugin::~BauhausPlugin()
{
    delete m_designerCore;
    Core::ICore::instance()->removeContextObject(m_context);
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool BauhausPlugin::initialize(const QStringList & /*arguments*/, QString *error_message/* = 0*/) // =0;
{
    Core::ICore *core = Core::ICore::instance();

    const int uid = core->uniqueIDManager()->uniqueIdentifier(QLatin1String(QmlDesigner::Constants::C_FORMEDITOR));
    const QList<int> context = QList<int>() << uid;

    const QList<int> switchContext = QList<int>() << uid
                                     << core->uniqueIDManager()->uniqueIdentifier(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);

    Core::ActionManager *am = core->actionManager();

    QAction *switchAction = new QAction(tr("Switch Text/Design"), this);
    Core::Command *command = am->registerAction(switchAction, QmlDesigner::Constants::SWITCH_TEXT_DESIGN, switchContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));

    m_designerCore = new QmlDesigner::IntegrationCore;
    m_pluginInstance = this;

#ifdef Q_OS_MAC
    const QString pluginPath = QCoreApplication::applicationDirPath() + "/../PlugIns/QmlDesigner";
#else
    const QString pluginPath = QCoreApplication::applicationDirPath() + "/../"
                               + QLatin1String(IDE_LIBRARY_BASENAME) + "/qmldesigner";
#endif

    m_designerCore->pluginManager()->setPluginPaths(QStringList() << pluginPath);

    createDesignModeWidget();
    connect(switchAction, SIGNAL(triggered()), this, SLOT(switchTextDesign()));

    addAutoReleasedObject(new SettingsPage);

    error_message->clear();

    return true;
}

void BauhausPlugin::createDesignModeWidget()
{
    Core::ICore *creatorCore = Core::ICore::instance();
    Core::ActionManager *actionManager = creatorCore->actionManager();
    m_editorManager = creatorCore->editorManager();
    Core::ActionContainer *editMenu = actionManager->actionContainer(Core::Constants::M_EDIT);

    m_mainWidget = new DesignModeWidget;

    m_context = new DesignModeContext(m_mainWidget);
    creatorCore->addContextObject(m_context);

    // Revert to saved
    actionManager->registerAction(m_revertToSavedAction,
                                      Core::Constants::REVERTTOSAVED, m_context->context());
    connect(m_revertToSavedAction, SIGNAL(triggered()), m_editorManager, SLOT(revertToSaved()));

    //Save
    actionManager->registerAction(m_saveAction, Core::Constants::SAVE, m_context->context());
    connect(m_saveAction, SIGNAL(triggered()), m_editorManager, SLOT(saveFile()));

    //Save As
    actionManager->registerAction(m_saveAsAction, Core::Constants::SAVEAS, m_context->context());
    connect(m_saveAsAction, SIGNAL(triggered()), m_editorManager, SLOT(saveFileAs()));

    //Close Editor
    actionManager->registerAction(m_closeCurrentEditorAction, Core::Constants::CLOSE, m_context->context());
    connect(m_closeCurrentEditorAction, SIGNAL(triggered()), m_editorManager, SLOT(closeEditor()));

    //Close All
    actionManager->registerAction(m_closeAllEditorsAction, Core::Constants::CLOSEALL, m_context->context());
    connect(m_closeAllEditorsAction, SIGNAL(triggered()), m_editorManager, SLOT(closeAllEditors()));

    //Close All Others Action
    actionManager->registerAction(m_closeOtherEditorsAction, Core::Constants::CLOSEOTHERS, m_context->context());
    connect(m_closeOtherEditorsAction, SIGNAL(triggered()), m_editorManager, SLOT(closeOtherEditors()));

    // Undo / Redo
    actionManager->registerAction(m_mainWidget->undoAction(), Core::Constants::UNDO, m_context->context());
    actionManager->registerAction(m_mainWidget->redoAction(), Core::Constants::REDO, m_context->context());

    Core::Command *command;
    command = actionManager->registerAction(m_mainWidget->deleteAction(),
                                            QmlDesigner::Constants::DELETE, m_context->context());
    command->setDefaultKeySequence(QKeySequence::Delete);
    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->cutAction(),
                                            Core::Constants::CUT, m_context->context());
    command->setDefaultKeySequence(QKeySequence::Cut);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->copyAction(),
                                            Core::Constants::COPY, m_context->context());
    command->setDefaultKeySequence(QKeySequence::Copy);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->pasteAction(),
                                            Core::Constants::PASTE, m_context->context());
    command->setDefaultKeySequence(QKeySequence::Paste);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->selectAllAction(),
                                            Core::Constants::SELECTALL, m_context->context());
    command->setDefaultKeySequence(QKeySequence::SelectAll);
    editMenu->addAction(command, Core::Constants::G_EDIT_SELECTALL);

    // add second shortcut to trigger delete
    QAction *deleteAction = new QAction(m_mainWidget);
    deleteAction->setShortcut(QKeySequence(QLatin1String("Backspace")));
    connect(deleteAction, SIGNAL(triggered()), m_mainWidget->deleteAction(),
            SIGNAL(triggered()));

    m_mainWidget->addAction(deleteAction);

    connect(m_editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateEditor(Core::IEditor*)));

    connect(m_editorManager, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(textEditorsClosed(QList<Core::IEditor*>)));

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(modeChanged(Core::IMode*)));
}

void BauhausPlugin::updateEditor(Core::IEditor *editor)
{
    Core::ICore *creatorCore = Core::ICore::instance();
    if (editor && editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
        && creatorCore->modeManager()->currentMode() == m_designMode)
    {
        m_mainWidget->showEditor(editor);
    }
}

void BauhausPlugin::modeChanged(Core::IMode *mode)
{
    if (mode == m_designMode) {
        m_isActive = true;
        m_mainWidget->showEditor(m_editorManager->currentEditor());
    } else {
        if (m_isActive) {
            m_isActive = false;
            m_mainWidget->showEditor(0);
        }
    }
}

void BauhausPlugin::textEditorsClosed(QList<Core::IEditor*> editors)
{
    m_mainWidget->closeEditors(editors);
}

// copied from EditorManager::updateActions
void BauhausPlugin::updateActions(Core::IEditor* editor)
{
    Core::IEditor *curEditor = editor;
    int openedCount = m_editorManager->openedEditors().count()
                      + m_editorManager->openedEditorsModel()->restoredEditors().count();

    QString fName;
    if (curEditor) {
        if (!curEditor->file()->fileName().isEmpty()) {
            QFileInfo fi(curEditor->file()->fileName());
            fName = fi.fileName();
        } else {
            fName = curEditor->displayName();
        }
    }

    m_saveAction->setEnabled(curEditor != 0 && curEditor->file()->isModified());
    m_saveAsAction->setEnabled(curEditor != 0 && curEditor->file()->isSaveAsAllowed());
    m_revertToSavedAction->setEnabled(curEditor != 0
                                      && !curEditor->file()->fileName().isEmpty()
                                      && curEditor->file()->isModified());

    QString quotedName;
    if (!fName.isEmpty())
        quotedName = '"' + fName + '"';
    m_saveAsAction->setText(tr("Save %1 As...").arg(quotedName));
    m_saveAction->setText(tr("&Save %1").arg(quotedName));
    m_revertToSavedAction->setText(tr("Revert %1 to Saved").arg(quotedName));

    m_closeCurrentEditorAction->setEnabled(curEditor != 0);
    m_closeCurrentEditorAction->setText(tr("Close %1").arg(quotedName));
    m_closeAllEditorsAction->setEnabled(openedCount > 0);
    m_closeOtherEditorsAction->setEnabled(openedCount > 1);
    m_closeOtherEditorsAction->setText((openedCount > 1 ? tr("Close All Except %1").arg(quotedName) : tr("Close Others")));
}

void BauhausPlugin::extensionsInitialized()
{
    m_designMode = ExtensionSystem::PluginManager::instance()->getObject<Core::DesignMode>();

    m_mimeTypes << "application/x-qml" << "application/javascript"
                << "application/x-javascript" << "text/javascript";

    m_designMode->registerDesignWidget(m_mainWidget, m_mimeTypes);
    connect(m_designMode, SIGNAL(actionsUpdated(Core::IEditor*)), SLOT(updateActions(Core::IEditor*)));
}

BauhausPlugin *BauhausPlugin::pluginInstance()
{
    return m_pluginInstance;
}

void BauhausPlugin::switchTextDesign()
{
    Core::ModeManager *modeManager = Core::ModeManager::instance();
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->currentEditor();


    if (modeManager->currentMode()->id() == Core::Constants::MODE_EDIT) {
        if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
            modeManager->activateMode(Core::Constants::MODE_DESIGN);
            m_mainWidget->setFocus();
        }
    } else if (modeManager->currentMode()->id() == Core::Constants::MODE_DESIGN) {
        modeManager->activateMode(Core::Constants::MODE_EDIT);
    }
}

DesignerSettings BauhausPlugin::settings() const
{
    return m_settings;
}

void BauhausPlugin::setSettings(const DesignerSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *settings = Core::ICore::instance()->settings())
            m_settings.toSettings(settings);
    }
}

}
}

Q_EXPORT_PLUGIN(QmlDesigner::Internal::BauhausPlugin)
