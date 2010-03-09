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

#include "designmodetoolbar.h"

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

Q_DECLARE_METATYPE(Core::IEditor*)

enum {
    debug = false
};

namespace Core {

/*!
  Mimic the look of the text editor toolbar as defined in e.g. EditorView::EditorView
  */
DesignModeToolBar::DesignModeToolBar(QWidget *parent) :
        QWidget(parent),
        m_editorList(new QComboBox),
        m_centerToolBar(0),
        m_rightToolBar(new QToolBar),
        m_closeButton(new QToolButton),
        m_lockButton(new QToolButton),
        m_goBackAction(new QAction(QIcon(QLatin1String(":/help/images/previous.png")), EditorManager::tr("Go Back"), parent)),
        m_goForwardAction(new QAction(QIcon(QLatin1String(":/help/images/next.png")), EditorManager::tr("Go Forward"), parent)),
        m_editor(0)
{
    ICore *core = ICore::instance();

    //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_editorsListModel = core->editorManager()->openedEditorsModel();

    // copied from EditorView::EditorView
    m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_editorList->setMinimumContentsLength(20);
    m_editorList->setModel(m_editorsListModel);
    m_editorList->setMaxVisibleItems(40);
    m_editorList->setContextMenuPolicy(Qt::CustomContextMenu);

    QToolBar *editorListToolBar = new QToolBar;
    editorListToolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    editorListToolBar->addWidget(m_editorList);

    m_lockButton->setAutoRaise(true);
    m_lockButton->setProperty("type", QLatin1String("dockbutton"));

    m_closeButton->setAutoRaise(true);
    m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    m_closeButton->setProperty("type", QLatin1String("dockbutton"));

//    ActionManager *am = core->actionManager();
//    EditorManager *em = core->editorManager();

// TODO back/FW buttons disabled for the time being, as the implementation would require changing editormanager.
//    QToolBar *backFwToolBar = new QToolBar;
//    backFwToolBar->addAction(m_goBackAction);
//    backFwToolBar->addAction(m_goForwardAction);
//    Command *cmd = am->registerAction(m_goBackAction, Constants::GO_BACK, editor->context());
//#ifdef Q_WS_MAC
//    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Left")));
//#else
//    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Left")));
//#endif
//    connect(m_goBackAction, SIGNAL(triggered()), em, SLOT(goBackInNavigationHistory()));
//    cmd = am->registerAction(m_goForwardAction, Constants::GO_FORWARD, editor->context());
//#ifdef Q_WS_MAC
//    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Right")));
//#else
//    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Right")));
//#endif
//    connect(m_goForwardAction, SIGNAL(triggered()), em, SLOT(goForwardInNavigationHistory()));

    m_rightToolBar->setLayoutDirection(Qt::RightToLeft);
    m_rightToolBar->addWidget(m_closeButton);
    m_rightToolBar->addWidget(m_lockButton);

    QHBoxLayout *toplayout = new QHBoxLayout(this);
    toplayout->setSpacing(0);
    toplayout->setMargin(0);
    toplayout->setContentsMargins(0,0,0,0);
    toplayout->addWidget(editorListToolBar);
    toplayout->addWidget(m_rightToolBar);

    connect(m_editorList, SIGNAL(activated(int)), this, SLOT(listSelectionActivated(int)));
    connect(m_editorList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(m_lockButton, SIGNAL(clicked()), this, SLOT(makeEditorWritable()));
    connect(m_closeButton, SIGNAL(clicked()), this, SIGNAL(closeClicked()));

    connect(core->editorManager(), SIGNAL(currentEditorChanged(IEditor*)), SLOT(updateEditorListSelection(IEditor*)));

    updateEditorStatus();
}

void DesignModeToolBar::setCenterToolBar(QWidget *toolBar)
{
    if (toolBar) {
        layout()->removeWidget(m_rightToolBar);
        layout()->addWidget(toolBar);
    }

    layout()->addWidget(m_rightToolBar);
}

void DesignModeToolBar::setEditor(IEditor *editor)
{
    m_editor = editor;
    m_editorList->setCurrentIndex(m_editorsListModel->indexOf(m_editor).row());
    connect(m_editor, SIGNAL(changed()), this, SLOT(updateEditorStatus()));
}

void DesignModeToolBar::updateEditorListSelection(IEditor *newSelection)
{
    if (newSelection) {
        m_editorList->setCurrentIndex(m_editorsListModel->indexOf(newSelection).row());
    }
}

void DesignModeToolBar::listSelectionActivated(int row)
{
    EditorManager *em = ICore::instance()->editorManager();
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

void DesignModeToolBar::listContextMenu(QPoint pos)
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

void DesignModeToolBar::makeEditorWritable()
{
    if (m_editor)
        ICore::instance()->editorManager()->makeEditorWritable(m_editor);
}

void DesignModeToolBar::updateEditorStatus()
{
    if (!m_editor || !m_editor->file())
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

} // Core
