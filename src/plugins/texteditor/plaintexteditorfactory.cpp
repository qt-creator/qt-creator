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

#include "plaintexteditor.h"
#include "plaintexteditorfactory.h"
#include "basetextdocument.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditoractionhandler.h"
#include "texteditorsettings.h"
#include "manager.h"
#include "highlightersettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/infobar.h>

#include <QCoreApplication>
#include <QDebug>

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
}

PlainTextEditorFactory::~PlainTextEditorFactory()
{
    delete m_actionHandler;
}

Core::Id PlainTextEditorFactory::id() const
{
    return Core::Id(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
}

QString PlainTextEditorFactory::displayName() const
{
    return qApp->translate("OpenWith::Editors", Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME);
}

Core::IEditor *PlainTextEditorFactory::createEditor(QWidget *parent)
{
    PlainTextEditorWidget *rc = new PlainTextEditorWidget(parent);
    TextEditorPlugin::instance()->initializeEditor(rc);
    connect(rc, SIGNAL(configured(Core::IEditor*)),
            this, SLOT(updateEditorInfoBar(Core::IEditor*)));
    updateEditorInfoBar(rc->editor());
    return rc->editor();
}

void PlainTextEditorFactory::updateEditorInfoBar(Core::IEditor *editor)
{
    PlainTextEditor *editorEditable = qobject_cast<PlainTextEditor *>(editor);
    if (editorEditable) {
        BaseTextDocument *file = qobject_cast<BaseTextDocument *>(editor->document());
        if (!file)
            return;
        PlainTextEditorWidget *textEditor = static_cast<PlainTextEditorWidget *>(editorEditable->editorWidget());
        Core::Id infoSyntaxDefinition(Constants::INFO_SYNTAX_DEFINITION);
        if (textEditor->isMissingSyntaxDefinition() &&
            !textEditor->ignoreMissingSyntaxDefinition() &&
            TextEditorSettings::instance()->highlighterSettings().alertWhenNoDefinition()) {
            if (file->hasHighlightWarning())
                return;
            Core::InfoBarEntry info(infoSyntaxDefinition,
                                    tr("A highlight definition was not found for this file. "
                                       "Would you like to try to find one?"));
            info.setCustomButtonInfo(tr("Show highlighter options..."),
                                     textEditor, SLOT(acceptMissingSyntaxDefinitionInfo()));
            info.setCancelButtonInfo(textEditor, SLOT(ignoreMissingSyntaxDefinitionInfo()));
            file->infoBar()->addInfo(info);
            file->setHighlightWarning(true);
            return;
        }
        if (!file->hasHighlightWarning())
            return;
        file->infoBar()->removeInfo(infoSyntaxDefinition);
        file->setHighlightWarning(false);
    }
}

void PlainTextEditorFactory::addMimeType(const QString &type)
{
    m_mimeTypes.append(type);
}

QStringList PlainTextEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
