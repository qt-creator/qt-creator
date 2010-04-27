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

#include "editorfactory.h"
#include "genericeditorconstants.h"
#include "editor.h"

#include <coreplugin/editormanager/editormanager.h>

using namespace GenericEditor;
using namespace Internal;

EditorFactory::EditorFactory(QObject *parent) :
    Core::IEditorFactory(parent)
{}

EditorFactory::~EditorFactory()
{}

Core::IEditor *EditorFactory::createEditor(QWidget *parent)
{
    Editor *genericEditor = createGenericEditor(parent);
    return genericEditor->editableInterface();
}

QString EditorFactory::id() const
{
    return QLatin1String(GenericEditor::Constants::GENERIC_EDITOR);
}

QStringList EditorFactory::mimeTypes() const
{ return m_mimeTypes; }

QString EditorFactory::displayName() const
{
    return tr(GenericEditor::Constants::GENERIC_EDITOR_DISPLAY_NAME);
}

Core::IFile *EditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

void EditorFactory::addMimeType(const QString &mimeType)
{ m_mimeTypes.append(mimeType); }
