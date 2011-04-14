/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "plaintexteditor.h"
#include "plaintexteditorfactory.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditoractionhandler.h"
#include "texteditorsettings.h"
#include "manager.h"
#include "highlightersettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QDebug>

using namespace TextEditor;
using namespace TextEditor::Internal;

PlainTextEditorFactory::PlainTextEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    m_actionHandler = new TextEditorActionHandler(
        TextEditor::Constants::C_TEXTEDITOR,
        TextEditorActionHandler::Format |
        TextEditorActionHandler::UnCommentSelection |
        TextEditorActionHandler::UnCollapseAll);
    m_mimeTypes << QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT);

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateEditorInfoBar(Core::IEditor*)));
}

PlainTextEditorFactory::~PlainTextEditorFactory()
{
    delete m_actionHandler;
}

QString PlainTextEditorFactory::id() const
{
    return QLatin1String(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
}

QString PlainTextEditorFactory::displayName() const
{
    return tr(Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME);
}

Core::IFile *PlainTextEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

Core::IEditor *PlainTextEditorFactory::createEditor(QWidget *parent)
{
    PlainTextEditorWidget *rc = new PlainTextEditorWidget(parent);
    TextEditorPlugin::instance()->initializeEditor(rc);
    connect(rc, SIGNAL(configured(Core::IEditor*)),
            this, SLOT(updateEditorInfoBar(Core::IEditor*)));
    return rc->editor();
}

void PlainTextEditorFactory::updateEditorInfoBar(Core::IEditor *editor)
{
    PlainTextEditor *editorEditable = qobject_cast<PlainTextEditor *>(editor);
    if (editorEditable) {
        PlainTextEditorWidget *textEditor = static_cast<PlainTextEditorWidget *>(editorEditable->editorWidget());
        if (textEditor->isMissingSyntaxDefinition() &&
            !textEditor->ignoreMissingSyntaxDefinition() &&
            TextEditorSettings::instance()->highlighterSettings().alertWhenNoDefinition()) {
            Core::EditorManager::instance()->showEditorInfoBar(
                Constants::INFO_SYNTAX_DEFINITION,
                tr("A highlight definition was not found for this file. "
                   "Would you like to try to find one?"),
                tr("Show highlighter options"),
                textEditor,
                SLOT(acceptMissingSyntaxDefinitionInfo()),
                SLOT(ignoreMissingSyntaxDefinitionInfo()));
            return;
        }
    }
    Core::EditorManager::instance()->hideEditorInfoBar(Constants::INFO_SYNTAX_DEFINITION);
}

void PlainTextEditorFactory::addMimeType(const QString &type)
{
    m_mimeTypes.append(type);
}

QStringList PlainTextEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
