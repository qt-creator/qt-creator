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

#include "designmode.h"
#include "qmldesignerconstants.h"
#include "designmodewidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QDebug>
#include <QPlainTextEdit>
#include <QFileInfo>

namespace QmlDesigner {
namespace Internal {

enum {
    debug = false
};

DesignModeCoreListener::DesignModeCoreListener(DesignMode *mode) :
        m_mode(mode)
{
}

bool DesignModeCoreListener::coreAboutToClose()
{
    // make sure settings are stored before actual program exit
    m_mode->currentEditorChanged(0);
    return true;
}

DesignMode::DesignMode() :
        IMode(),
        m_mainWidget(new DesignModeWidget(this)),
        m_coreListener(new DesignModeCoreListener(this)),
        m_isActive(false),
        m_revertToSavedAction(new QAction(this)),
        m_saveAction(new QAction(this)),
        m_saveAsAction(new QAction(this)),
        m_closeCurrentEditorAction(new QAction(this)),
        m_closeAllEditorsAction(new QAction(this)),
        m_closeOtherEditorsAction(new QAction(this))
{
    Core::ICore *creatorCore = Core::ICore::instance();
    Core::ModeManager *modeManager = creatorCore->modeManager();

    connect(modeManager, SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(modeChanged(Core::IMode*)));

    ExtensionSystem::PluginManager::instance()->addObject(m_coreListener);

    Core::ActionManager *actionManager = creatorCore->actionManager();
    Core::EditorManager *editorManager = creatorCore->editorManager();

    // Undo / Redo
    actionManager->registerAction(m_mainWidget->undoAction(), Core::Constants::UNDO, context());
    actionManager->registerAction(m_mainWidget->redoAction(), Core::Constants::REDO, context());

    // Revert to saved
    actionManager->registerAction(m_revertToSavedAction,
                                      Core::Constants::REVERTTOSAVED, context());
    connect(m_revertToSavedAction, SIGNAL(triggered()), editorManager, SLOT(revertToSaved()));

    //Save
    actionManager->registerAction(m_saveAction, Core::Constants::SAVE, context());
    connect(m_saveAction, SIGNAL(triggered()), editorManager, SLOT(saveFile()));

    //Save As
    actionManager->registerAction(m_saveAsAction, Core::Constants::SAVEAS, context());
    connect(m_saveAsAction, SIGNAL(triggered()), editorManager, SLOT(saveFileAs()));

    //Close Editor
    actionManager->registerAction(m_closeCurrentEditorAction, Core::Constants::CLOSE, context());
    connect(m_closeCurrentEditorAction, SIGNAL(triggered()), editorManager, SLOT(closeEditor()));

    //Close All
    actionManager->registerAction(m_closeAllEditorsAction, Core::Constants::CLOSEALL, context());
    connect(m_closeAllEditorsAction, SIGNAL(triggered()), editorManager, SLOT(closeAllEditors()));

    //Close All Others Action
    actionManager->registerAction(m_closeOtherEditorsAction, Core::Constants::CLOSEOTHERS, context());
    connect(m_closeOtherEditorsAction, SIGNAL(triggered()), editorManager, SLOT(closeOtherEditors()));

    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(textEditorsClosed(QList<Core::IEditor*>)));

    Core::ActionContainer *editMenu = actionManager->actionContainer(Core::Constants::M_EDIT);

    Core::Command *command;
    command = actionManager->registerAction(m_mainWidget->deleteAction(),
                                            QmlDesigner::Constants::DELETE, context());
    command->setDefaultKeySequence(QKeySequence::Delete);
    command->setAttribute(Core::Command::CA_Hide); // don't show delete in other modes
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->cutAction(),
                                            Core::Constants::CUT, context());
    command->setDefaultKeySequence(QKeySequence::Cut);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->copyAction(),
                                            Core::Constants::COPY, context());
    command->setDefaultKeySequence(QKeySequence::Copy);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    command = actionManager->registerAction(m_mainWidget->pasteAction(),
                                            Core::Constants::PASTE, context());
    command->setDefaultKeySequence(QKeySequence::Paste);
    editMenu->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    // add second shortcut to trigger delete
    QAction *deleteAction = new QAction(m_mainWidget);
    deleteAction->setShortcut(QKeySequence(QLatin1String("Backspace")));
    connect(deleteAction, SIGNAL(triggered()), m_mainWidget->deleteAction(),
            SIGNAL(triggered()));

    m_mainWidget->addAction(deleteAction);

    updateActions();
}

DesignMode::~DesignMode()
{
    delete m_mainWidget;
    ExtensionSystem::PluginManager::instance()->removeObject(m_coreListener);
    delete m_coreListener;
}

QList<int> DesignMode::context() const
{
    static QList<int> contexts = QList<int>() <<
        Core::UniqueIDManager::instance()->uniqueIdentifier(Constants::C_DESIGN_MODE);
    return contexts;
}

QWidget *DesignMode::widget()
{
    return m_mainWidget;
}

QString DesignMode::name() const
{
    return tr(Constants::DESIGN_MODE_NAME);
}

QIcon DesignMode::icon() const
{
    return QIcon(QLatin1String(":/qmldesigner/images/mode_Design.png"));
}

int DesignMode::priority() const
{
    return Constants::DESIGN_MODE_PRIORITY;
}

const char *DesignMode::uniqueModeName() const
{
    return Constants::DESIGN_MODE_NAME;
}

void DesignMode::textEditorsClosed(QList<Core::IEditor*> editors)
{
    m_mainWidget->closeEditors(editors);
}

void DesignMode::modeChanged(Core::IMode *mode)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << ((mode == this) ? "Design mode" : "other mode");
    if (mode == this) {
        m_isActive = true;
        m_mainWidget->showEditor(m_currentEditor.data());
    } else {
        if (m_isActive) {
            m_isActive = false;
//            m_mainWidget->showEditor(0);
        }
    }
}

void DesignMode::currentEditorChanged(Core::IEditor *editor)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << editor;

    if (m_currentEditor.data() == editor)
        return;

    if (m_currentEditor)
        disconnect(m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

    m_currentEditor = QWeakPointer<Core::IEditor>(editor);

    if (m_currentEditor)
        connect(m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

    updateActions();
}

void DesignMode::makeCurrentEditorWritable()
{
    Core::ICore *creatorCore = Core::ICore::instance();
    if (m_currentEditor)
        creatorCore->editorManager()->makeEditorWritable(m_currentEditor.data());
}

// copied from EditorManager::updateActions
void DesignMode::updateActions()
{
    Core::ICore *creatorCore = Core::ICore::instance();
    Core::EditorManager *editorManager = creatorCore->editorManager();

    Core::IEditor *curEditor = m_currentEditor.data();
    int openedCount = editorManager->openedEditors().count()
                      + editorManager->openedEditorsModel()->restoredEditorCount();

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

} // namespace Internal
} // namespace QmlDesigner
