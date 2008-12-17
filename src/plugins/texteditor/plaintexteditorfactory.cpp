/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "plaintexteditor.h"
#include "plaintexteditorfactory.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditoractionhandler.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

using namespace TextEditor;
using namespace TextEditor::Internal;

PlainTextEditorFactory::PlainTextEditorFactory(QObject *parent)   :
    Core::IEditorFactory(parent),
    m_kind(Core::Constants::K_DEFAULT_TEXT_EDITOR)
{
    m_actionHandler = new TextEditorActionHandler(TextEditorPlugin::core(),
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
    Core::ICore *core = TextEditorPlugin::core();
    Core::IEditor *iface = core->editorManager()->openEditor(fileName, kind());
    return iface ? iface->file() : 0;
}

Core::IEditor *PlainTextEditorFactory::createEditor(QWidget *parent)
{
    PlainTextEditor *rc =  new PlainTextEditor(parent);
    TextEditorPlugin::instance()->initializeEditor(rc);
    return rc->editableInterface();
}

QStringList PlainTextEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
