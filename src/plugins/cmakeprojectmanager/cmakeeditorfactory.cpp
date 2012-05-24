/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cmakeeditorfactory.h"
#include "cmakeprojectconstants.h"
#include "cmakeeditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeEditorFactory::CMakeEditorFactory(CMakeManager *manager)
    : m_mimeTypes(QStringList() << QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE)),
      m_manager(manager)
{
    using namespace Core;
    using namespace TextEditor;

    m_actionHandler =
            new TextEditorActionHandler(Constants::C_CMAKEEDITOR,
            TextEditorActionHandler::UnCommentSelection
            | TextEditorActionHandler::JumpToFileUnderCursor);

    ActionContainer *contextMenu = Core::ActionManager::createMenu(Constants::M_CONTEXT);
    Command *cmd;
    Context cmakeEditorContext = Context(Constants::C_CMAKEEDITOR);

    cmd = Core::ActionManager::command(TextEditor::Constants::JUMP_TO_FILE_UNDER_CURSOR);
    contextMenu->addAction(cmd);

    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    contextMenu->addAction(Core::ActionManager::registerAction(separator,
                  Id(Constants::SEPARATOR), cmakeEditorContext));

    cmd = Core::ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);
}

Core::Id CMakeEditorFactory::id() const
{
    return CMakeProjectManager::Constants::CMAKE_EDITOR_ID;
}

QString CMakeEditorFactory::displayName() const
{
    return tr(CMakeProjectManager::Constants::CMAKE_EDITOR_DISPLAY_NAME);
}

Core::IEditor *CMakeEditorFactory::createEditor(QWidget *parent)
{
    CMakeEditorWidget *rc = new CMakeEditorWidget(parent, this, m_actionHandler);
    TextEditor::TextEditorSettings::instance()->initializeEditor(rc);
    return rc->editor();
}

QStringList CMakeEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
