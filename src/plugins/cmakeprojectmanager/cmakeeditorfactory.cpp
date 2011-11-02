/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditorsettings.h>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeEditorFactory::CMakeEditorFactory(CMakeManager *manager, TextEditor::TextEditorActionHandler *handler)
    : m_mimeTypes(QStringList() << QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE)),
      m_manager(manager),
      m_actionHandler(handler)
{

}

Core::Id CMakeEditorFactory::id() const
{
    return CMakeProjectManager::Constants::CMAKE_EDITOR_ID;
}

QString CMakeEditorFactory::displayName() const
{
    return tr(CMakeProjectManager::Constants::CMAKE_EDITOR_DISPLAY_NAME);
}

Core::IFile *CMakeEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->file() : 0;
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
