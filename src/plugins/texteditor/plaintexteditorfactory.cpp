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

#include "plaintexteditor.h"
#include "plaintexteditorfactory.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditoractionhandler.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

using namespace TextEditor;
using namespace TextEditor::Internal;

PlainTextEditorFactory::PlainTextEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent),
    m_kind(Core::Constants::K_DEFAULT_TEXT_EDITOR)
{
    m_actionHandler = new TextEditorActionHandler(
        QLatin1String(TextEditor::Constants::C_TEXTEDITOR),
        TextEditorActionHandler::Format);
    m_mimeTypes << QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT)
                << QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_XML);
}

PlainTextEditorFactory::~PlainTextEditorFactory()
{
    delete m_actionHandler;
}

QString PlainTextEditorFactory::kind() const
{
    return m_kind;
}

Core::IFile *PlainTextEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, kind());
    return iface ? iface->file() : 0;
}

Core::IEditor *PlainTextEditorFactory::createEditor(QWidget *parent)
{
    PlainTextEditor *rc = new PlainTextEditor(parent);
    TextEditorPlugin::instance()->initializeEditor(rc);
    return rc->editableInterface();
}

QStringList PlainTextEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
