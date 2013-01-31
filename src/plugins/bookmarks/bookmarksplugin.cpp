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

#include "bookmarksplugin.h"
#include "bookmarkmanager.h"
#include "bookmarks_global.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <QtPlugin>
#include <QDebug>

#include <QMenu>

using namespace Bookmarks::Constants;
using namespace Bookmarks::Internal;
using namespace TextEditor;

BookmarksPlugin *BookmarksPlugin::m_instance = 0;

BookmarksPlugin::BookmarksPlugin()
    : m_bookmarkManager(0),
      m_bookmarkMarginActionLineNumber(0)
{
    m_instance = this;
}

void BookmarksPlugin::extensionsInitialized()
{
}

bool BookmarksPlugin::initialize(const QStringList & /*arguments*/, QString *)
{
    Core::Context textcontext(TextEditor::Constants::C_TEXTEDITOR);
    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    Core::ActionContainer *mtools = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *mbm = Core::ActionManager::createMenu(Core::Id(BOOKMARKS_MENU));
    mbm->menu()->setTitle(tr("&Bookmarks"));
    mtools->addMenu(mbm);

    //Toggle
    m_toggleAction = new QAction(tr("Toggle Bookmark"), this);
    Core::Command *cmd =
        Core::ActionManager::registerAction(m_toggleAction, BOOKMARKS_TOGGLE_ACTION, textcontext);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+M") : tr("Ctrl+M")));
    mbm->addAction(cmd);

    mbm->addSeparator(textcontext);

    //Previous
    m_prevAction = new QAction(tr("Previous Bookmark"), this);
    cmd = Core::ActionManager::registerAction(m_prevAction, BOOKMARKS_PREV_ACTION, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+,") : tr("Ctrl+,")));
    mbm->addAction(cmd);

    //Next
    m_nextAction = new QAction(tr("Next Bookmark"), this);
    cmd = Core::ActionManager::registerAction(m_nextAction, BOOKMARKS_NEXT_ACTION, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+.") : tr("Ctrl+.")));
    mbm->addAction(cmd);

    mbm->addSeparator(globalcontext);

    //Previous Doc
    m_docPrevAction = new QAction(tr("Previous Bookmark in Document"), this);
    cmd = Core::ActionManager::registerAction(m_docPrevAction, BOOKMARKS_PREVDOC_ACTION, globalcontext);
    mbm->addAction(cmd);

    //Next Doc
    m_docNextAction = new QAction(tr("Next Bookmark in Document"), this);
    cmd = Core::ActionManager::registerAction(m_docNextAction, BOOKMARKS_NEXTDOC_ACTION, globalcontext);
    mbm->addAction(cmd);

    m_editNoteAction = new QAction(tr("Edit Bookmark Note"), this);

    m_bookmarkManager = new BookmarkManager;

    connect(m_toggleAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(toggleBookmark()));
    connect(m_prevAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(prev()));
    connect(m_nextAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(next()));
    connect(m_docPrevAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(prevInDocument()));
    connect(m_docNextAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(nextInDocument()));
    connect(m_editNoteAction, SIGNAL(triggered()), this, SLOT(bookmarkEditNoteActionTriggered()));
    connect(m_bookmarkManager, SIGNAL(updateActions(int)), this, SLOT(updateActions(int)));
    updateActions(m_bookmarkManager->state());
    addAutoReleasedObject(new BookmarkViewFactory(m_bookmarkManager));

    m_bookmarkMarginAction = new QAction(this);
    m_bookmarkMarginAction->setText(tr("Toggle Bookmark"));
    connect(m_bookmarkMarginAction, SIGNAL(triggered()),
        this, SLOT(bookmarkMarginActionTriggered()));

    // EditorManager
    QObject *editorManager = Core::ICore::editorManager();
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    return true;
}

BookmarksPlugin::~BookmarksPlugin()
{
    delete m_bookmarkManager;
}

void BookmarksPlugin::updateActions(int state)
{
    const bool hasbm    = state >= BookmarkManager::HasBookMarks;
    const bool hasdocbm = state == BookmarkManager::HasBookmarksInDocument;

    m_toggleAction->setEnabled(true);
    m_prevAction->setEnabled(hasbm);
    m_nextAction->setEnabled(hasbm);
    m_docPrevAction->setEnabled(hasdocbm);
    m_docNextAction->setEnabled(hasdocbm);
}

void BookmarksPlugin::editorOpened(Core::IEditor *editor)
{
    if (qobject_cast<ITextEditor *>(editor)) {
        connect(editor, SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
                this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));

        connect(editor,
                SIGNAL(markRequested(TextEditor::ITextEditor*,int,
                                     TextEditor::ITextEditor::MarkRequestKind)),
                m_bookmarkManager,
                SLOT(handleBookmarkRequest(TextEditor::ITextEditor*,int,
                                           TextEditor::ITextEditor::MarkRequestKind)));
        connect(editor,
                SIGNAL(markTooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
                m_bookmarkManager,
                SLOT(handleBookmarkTooltipRequest(TextEditor::ITextEditor*,QPoint,int)));
    }
}

void BookmarksPlugin::editorAboutToClose(Core::IEditor *editor)
{
    if (qobject_cast<ITextEditor *>(editor)) {
        disconnect(editor, SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
                this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
    }
}

void BookmarksPlugin::requestContextMenu(TextEditor::ITextEditor *editor,
    int lineNumber, QMenu *menu)
{
    m_bookmarkMarginActionLineNumber = lineNumber;
    m_bookmarkMarginActionFileName = editor->document()->fileName();

    menu->addAction(m_bookmarkMarginAction);
    if (m_bookmarkManager->hasBookmarkInPosition(m_bookmarkMarginActionFileName, m_bookmarkMarginActionLineNumber))
        menu->addAction(m_editNoteAction);
}

void BookmarksPlugin::bookmarkMarginActionTriggered()
{
    m_bookmarkManager->toggleBookmark(m_bookmarkMarginActionFileName,
                                      m_bookmarkMarginActionLineNumber);
}

void BookmarksPlugin::bookmarkEditNoteActionTriggered()
{
    m_bookmarkManager->editNote(m_bookmarkMarginActionFileName, m_bookmarkMarginActionLineNumber);
}

Q_EXPORT_PLUGIN(BookmarksPlugin)
