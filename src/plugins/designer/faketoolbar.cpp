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

#include "faketoolbar.h"

#include <designerconstants.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/modemanager.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <QtCore/QSettings>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QMenu>
#include <QtGui/QClipboard>
#include <QtGui/QLabel>
#include <QtGui/QToolBar>

using Core::MiniSplitter;
using Core::IEditor;
using Core::EditorManager;

Q_DECLARE_METATYPE(Core::IEditor*)

enum {
    debug = false
};

namespace Designer {
namespace Internal {

/*!
  Mimic the look of the text editor toolbar as defined in e.g. EditorView::EditorView
  */
FakeToolBar::FakeToolBar(Core::IEditor *editor, QWidget *toolbar, QWidget *parent) :
        QWidget(parent),
        m_editorList(new QComboBox),
        m_closeButton(new QToolButton),
        m_lockButton(new QToolButton),
        m_goBackAction(new QAction(QIcon(QLatin1String(":/help/images/previous.png")), EditorManager::tr("Go Back"), parent)),
        m_goForwardAction(new QAction(QIcon(QLatin1String(":/help/images/next.png")), EditorManager::tr("Go Forward"), parent)),
        m_editor(editor)
{
    Core::ICore *core = Core::ICore::instance();

    //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_editorsListModel = core->editorManager()->openedEditorsModel();

    // copied from EditorView::EditorView
    m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_editorList->setMinimumContentsLength(20);
    m_editorList->setModel(m_editorsListModel);
    m_editorList->setMaxVisibleItems(40);
    m_editorList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_editorList->setCurrentIndex(m_editorsListModel->indexOf(m_editor).row());

    QToolBar *editorListToolBar = new QToolBar;
    editorListToolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    editorListToolBar->addWidget(m_editorList);

    m_lockButton->setAutoRaise(true);
    m_lockButton->setProperty("type", QLatin1String("dockbutton"));

    m_closeButton->setAutoRaise(true);
    m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    m_closeButton->setProperty("type", QLatin1String("dockbutton"));

//    Core::ActionManager *am = core->actionManager();
//    Core::EditorManager *em = core->editorManager();

// TODO back/FW buttons disabled for the time being, as the implementation would require changing editormanager.
//
//    QToolBar *backFwToolBar = new QToolBar;
//    backFwToolBar->addAction(m_goBackAction);
//    backFwToolBar->addAction(m_goForwardAction);
//    Core::Command *cmd = am->registerAction(m_goBackAction, Core::Constants::GO_BACK, editor->context());
//#ifdef Q_WS_MAC
//    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Left")));
//#else
//    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Left")));
//#endif
//    connect(m_goBackAction, SIGNAL(triggered()), em, SLOT(goBackInNavigationHistory()));
//    cmd = am->registerAction(m_goForwardAction, Core::Constants::GO_FORWARD, editor->context());
//#ifdef Q_WS_MAC
//    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Right")));
//#else
//    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Right")));
//#endif
//    connect(m_goForwardAction, SIGNAL(triggered()), em, SLOT(goForwardInNavigationHistory()));

    QToolBar *rightToolBar = new QToolBar;
    rightToolBar->setLayoutDirection(Qt::RightToLeft);
    rightToolBar->addWidget(m_closeButton);
    rightToolBar->addWidget(m_lockButton);

    QHBoxLayout *toplayout = new QHBoxLayout(this);
    toplayout->setSpacing(0);
    toplayout->setMargin(0);
    toplayout->setContentsMargins(0,0,0,0);

//    toplayout->addWidget(backFwToolBar);
    toplayout->addWidget(editorListToolBar);
    toplayout->addWidget(toolbar);
    toplayout->addWidget(rightToolBar);

    connect(m_editorList, SIGNAL(activated(int)), this, SLOT(listSelectionActivated(int)));
    connect(m_editorList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(m_lockButton, SIGNAL(clicked()), this, SLOT(makeEditorWritable()));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(close()));

    connect(m_editor, SIGNAL(changed()), this, SLOT(updateEditorStatus()));
    connect(core->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(updateEditorListSelection(Core::IEditor*)));

    updateEditorStatus();
}

void FakeToolBar::updateEditorListSelection(Core::IEditor *newSelection)
{
    if (newSelection) {
        m_editorList->setCurrentIndex(m_editorsListModel->indexOf(newSelection).row());
    }
}

void FakeToolBar::close()
{
    // instead of closing & deleting the visual editor, we want to go to edit mode and
    // close the xml file instead.
    Core::ICore *core = Core::ICore::instance();

    Core::IEditor *editor = core->editorManager()->currentEditor();
    if (editor && editor->id() == Designer::Constants::K_DESIGNER_XML_EDITOR_ID
        && editor->file() == m_editor->file())
    {
        core->editorManager()->closeEditors(QList<Core::IEditor*>() << editor);
    }
    core->modeManager()->activateMode(Core::Constants::MODE_EDIT);
}

void FakeToolBar::listSelectionActivated(int row)
{
    Core::EditorManager *em = Core::ICore::instance()->editorManager();
    QAbstractItemModel *model = m_editorList->model();

    const QModelIndex modelIndex = model->index(row, 0);
    IEditor *editor = model->data(modelIndex, Qt::UserRole).value<IEditor*>();
    if (editor) {
        if (editor != em->currentEditor())
            em->activateEditor(editor, EditorManager::NoModeSwitch);
    } else {
        QString fileName = model->data(modelIndex, Qt::UserRole + 1).toString();
        QByteArray kind = model->data(modelIndex, Qt::UserRole + 2).toByteArray();
        editor = em->openEditor(fileName, kind, EditorManager::NoModeSwitch);
    }
    if (editor) {
        m_editorList->setCurrentIndex(m_editorsListModel->indexOf(editor).row());
    }
}

void FakeToolBar::listContextMenu(QPoint pos)
{
    QModelIndex index = m_editorsListModel->index(m_editorList->currentIndex(), 0);
    QString fileName = m_editorsListModel->data(index, Qt::UserRole + 1).toString();
    if (fileName.isEmpty())
        return;
    QMenu menu;
    menu.addAction(tr("Copy full path to clipboard"));
    if (menu.exec(m_editorList->mapToGlobal(pos))) {
        QApplication::clipboard()->setText(fileName);
    }
}

void FakeToolBar::makeEditorWritable()
{
    Core::ICore::instance()->editorManager()->makeEditorWritable(m_editor);
}

void FakeToolBar::updateEditorStatus()
{
    if (!m_editor->file())
        return;

    if (m_editor->file()->isReadOnly()) {
        m_lockButton->setIcon(m_editorsListModel->lockedIcon());
        m_lockButton->setEnabled(!m_editor->file()->fileName().isEmpty());
        m_lockButton->setToolTip(tr("Make writable"));
    } else {
        m_lockButton->setIcon(m_editorsListModel->unlockedIcon());
        m_lockButton->setEnabled(false);
        m_lockButton->setToolTip(tr("File is writable"));
    }
    m_editorList->setToolTip(
            m_editor->file()->fileName().isEmpty()
            ? m_editor->displayName()
                : QDir::toNativeSeparators(m_editor->file()->fileName())
                );
}

} // Internal
} // Designer
