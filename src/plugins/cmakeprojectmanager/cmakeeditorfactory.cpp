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

    contextMenu->addSeparator(cmakeEditorContext);

    cmd = Core::ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);
}

Core::Id CMakeEditorFactory::id() const
{
    return Core::Id(CMakeProjectManager::Constants::CMAKE_EDITOR_ID);
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
