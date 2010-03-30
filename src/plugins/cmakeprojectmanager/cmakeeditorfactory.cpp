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

CMakeEditorFactory::~CMakeEditorFactory()
{
}

QString CMakeEditorFactory::id() const
{
    return QLatin1String(CMakeProjectManager::Constants::CMAKE_EDITOR_ID);
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
    CMakeEditor *rc = new CMakeEditor(parent, this, m_actionHandler);
    TextEditor::TextEditorSettings::instance()->initializeEditor(rc);
    return rc->editableInterface();
}

QStringList CMakeEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
