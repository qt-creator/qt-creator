/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "plaintexteditorfactory.h"
#include "basetexteditor.h"
#include "basetextdocument.h"
#include "normalindenter.h"
#include "texteditoractionhandler.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/infobar.h>

#include <QCoreApplication>
#include <QDebug>

using namespace TextEditor;
using namespace TextEditor::Internal;

PlainTextEditorFactory::PlainTextEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    setId(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME));
    addMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));

    new TextEditorActionHandler(this,
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
        TextEditorActionHandler::Format |
        TextEditorActionHandler::UnCommentSelection |
        TextEditorActionHandler::UnCollapseAll);
}

Core::IEditor *PlainTextEditorFactory::createEditor()
{
    BaseTextDocumentPtr doc(new BaseTextDocument(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID));
    doc->setIndenter(new NormalIndenter);
    auto widget = new BaseTextEditorWidget;
    widget->setTextDocument(doc);
    widget->setupAsPlainEditor();
    connect(widget, &BaseTextEditorWidget::configured,
            this, &PlainTextEditorFactory::updateEditorInfoBar);
    updateEditorInfoBar(widget->editor());
    return widget->editor();
}

/*!
 * Test if syntax highlighter is available (or unneeded) for \a editor.
 * If not found, show a warning with a link to the relevant settings page.
 */
void PlainTextEditorFactory::updateEditorInfoBar(Core::IEditor *editor)
{
    BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(editor);
    if (textEditor) {
        Core::IDocument *file = editor->document();
        if (!file)
            return;
        BaseTextEditorWidget *widget = textEditor->editorWidget();
        Core::Id infoSyntaxDefinition(Constants::INFO_SYNTAX_DEFINITION);
        Core::InfoBar *infoBar = file->infoBar();
        if (!widget->isMissingSyntaxDefinition()) {
            infoBar->removeInfo(infoSyntaxDefinition);
        } else if (infoBar->canInfoBeAdded(infoSyntaxDefinition)) {
            Core::InfoBarEntry info(infoSyntaxDefinition,
                                    tr("A highlight definition was not found for this file. "
                                       "Would you like to try to find one?"),
                                    Core::InfoBarEntry::GlobalSuppressionEnabled);
            info.setCustomButtonInfo(tr("Show Highlighter Options..."),
                                     widget, SLOT(acceptMissingSyntaxDefinitionInfo()));
            infoBar->addInfo(info);
        }
    }
}
