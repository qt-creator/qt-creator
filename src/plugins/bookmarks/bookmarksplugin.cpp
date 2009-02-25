/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "bookmarksplugin.h"
#include "bookmarkmanager.h"
#include "bookmarks_global.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>

#include <QtGui/QMenu>

using namespace Bookmarks::Constants;
using namespace Bookmarks::Internal;
using namespace TextEditor;

BookmarksPlugin *BookmarksPlugin::m_instance = 0;

BookmarksPlugin::BookmarksPlugin()
    : m_bookmarkManager(0)
{
    m_instance = this;
}

void BookmarksPlugin::extensionsInitialized()
{
}

bool BookmarksPlugin::initialize(const QStringList & /*arguments*/, QString *)
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    QList<int> context = QList<int>() << core->uniqueIDManager()->
        uniqueIdentifier(Constants::BOOKMARKS_CONTEXT);
    QList<int> textcontext, globalcontext;
    textcontext << core->uniqueIDManager()->
        uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
    globalcontext << Core::Constants::C_GLOBAL_ID;

    Core::ActionContainer *mtools =
        am->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *mbm =
        am->createMenu(QLatin1String(BOOKMARKS_MENU));
    mbm->menu()->setTitle(tr("&Bookmarks"));
    mtools->addMenu(mbm);

    //Toggle
    m_toggleAction = new QAction(tr("Toggle Bookmark"), this);
    Core::Command *cmd =
        am->registerAction(m_toggleAction, BOOKMARKS_TOGGLE_ACTION, textcontext);
#ifndef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+M")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Meta+M")));
#endif
    mbm->addAction(cmd);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Bookmarks.Sep.Toggle"), textcontext);
    mbm->addAction(cmd);

    // Move Up
    m_moveUpAction = new QAction(tr("Move Up"), this);
    cmd = am->registerAction(m_moveUpAction, BOOKMARKS_MOVEUP_ACTION, context);
    mbm->addAction(cmd);

    // Move Down
    m_moveDownAction = new QAction(tr("Move Down"), this);
    cmd = am->registerAction(m_moveDownAction, BOOKMARKS_MOVEDOWN_ACTION, context);
    mbm->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Bookmarks.Sep.Navigation"), context);
    mbm->addAction(cmd);

    //Previous
    m_prevAction = new QAction(tr("Previous Bookmark"), this);
    cmd = am->registerAction(m_prevAction, BOOKMARKS_PREV_ACTION, globalcontext);
#ifndef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+,")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Meta+,")));
#endif
    mbm->addAction(cmd);

    //Next
    m_nextAction = new QAction(tr("Next Bookmark"), this);
    cmd = am->registerAction(m_nextAction, BOOKMARKS_NEXT_ACTION, globalcontext);
#ifndef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+.")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Meta+.")));
#endif
    mbm->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Bookmarks.Sep.DirNavigation"), globalcontext);
    mbm->addAction(cmd);

    //Previous Doc
    m_docPrevAction = new QAction(tr("Previous Bookmark In Document"), this);
    cmd = am->registerAction(m_docPrevAction, BOOKMARKS_PREVDOC_ACTION, globalcontext);
    mbm->addAction(cmd);

    //Next Doc
    m_docNextAction = new QAction(tr("Next Bookmark In Document"), this);
    cmd = am->registerAction(m_docNextAction, BOOKMARKS_NEXTDOC_ACTION, globalcontext);
    mbm->addAction(cmd);

    m_bookmarkManager = new BookmarkManager;

    connect(m_toggleAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(toggleBookmark()));
    connect(m_prevAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(prev()));
    connect(m_nextAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(next()));
    connect(m_docPrevAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(prevInDocument()));
    connect(m_docNextAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(nextInDocument()));
    connect(m_moveUpAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(moveUp()));
    connect(m_moveDownAction, SIGNAL(triggered()), m_bookmarkManager, SLOT(moveDown()));
    connect(m_bookmarkManager, SIGNAL(updateActions(int)), this, SLOT(updateActions(int)));
    updateActions(m_bookmarkManager->state());
    addAutoReleasedObject(new BookmarkViewFactory(m_bookmarkManager));

    m_bookmarkMarginAction = new QAction(this);
    m_bookmarkMarginAction->setText("Toggle Bookmark");
    //m_bookmarkAction->setIcon(QIcon(":/gdbdebugger/images/breakpoint.svg"));
    connect(m_bookmarkMarginAction, SIGNAL(triggered()),
        this, SLOT(bookmarkMarginActionTriggered()));

    // EditorManager
    QObject *editorManager = core->editorManager();
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
    m_moveUpAction->setEnabled(hasbm);
    m_moveDownAction->setEnabled(hasbm);
}

void BookmarksPlugin::editorOpened(Core::IEditor *editor)
{
    if (qobject_cast<ITextEditor *>(editor)) {
        connect(editor, SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
                this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
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
    m_bookmarkMarginActionFileName = editor->file()->fileName();
    menu->addAction(m_bookmarkMarginAction);
}

void BookmarksPlugin::bookmarkMarginActionTriggered()
{
    m_bookmarkManager->toggleBookmark(
        m_bookmarkMarginActionFileName,
        m_bookmarkMarginActionLineNumber
    );
}

Q_EXPORT_PLUGIN(BookmarksPlugin)
