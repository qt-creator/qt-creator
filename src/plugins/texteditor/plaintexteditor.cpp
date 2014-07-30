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

#include "plaintexteditor.h"
#include "tabsettings.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"
#include "basetextdocument.h"
#include "normalindenter.h"
#include "highlighterutils.h"
#include <texteditor/generichighlighter/context.h>
#include <texteditor/generichighlighter/highlightdefinition.h>
#include <texteditor/generichighlighter/highlighter.h>
#include <texteditor/generichighlighter/highlightersettings.h>
#include <texteditor/generichighlighter/manager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

using namespace Core;
using namespace TextEditor::Internal;

/*!
    \class TextEditor::PlainTextEditor
    \brief The PlainTextEditor class is a text editor implementation that adds highlighting
    by using the Kate text highlighting definitions, and some basic auto indentation.

    It is the default editor for text files used by \QC, if no other editor implementation
    matches the MIME type. The corresponding document is implemented in PlainTextDocument,
    the corresponding widget is implemented in PlainTextEditorWidget.
*/

namespace TextEditor {

PlainTextEditor::PlainTextEditor(PlainTextEditorWidget *editor)
  : BaseTextEditor(editor)
{
    setContext(Core::Context(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                             TextEditor::Constants::C_TEXTEDITOR));
    setDuplicateSupported(true);
}

PlainTextEditorWidget::PlainTextEditorWidget(PlainTextDocument *doc, QWidget *parent)
    : BaseTextEditorWidget(doc, parent)
{
    ctor();
}

PlainTextEditorWidget::PlainTextEditorWidget(PlainTextEditorWidget *other)
    : BaseTextEditorWidget(other)
{
    ctor();
}

void PlainTextEditorWidget::ctor()
{
    setRevisionsVisible(true);
    setMarksVisible(true);
    setLineSeparatorsAllowed(true);

    baseTextDocument()->setMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));

    connect(baseTextDocument(), SIGNAL(filePathChanged(QString,QString)),
            this, SLOT(configureMimeType()));
    connect(Manager::instance(), SIGNAL(mimeTypesRegistered()), this, SLOT(configureMimeType()));
}

IEditor *PlainTextEditor::duplicate()
{
    PlainTextEditorWidget *newWidget = new PlainTextEditorWidget(
                qobject_cast<PlainTextEditorWidget *>(editorWidget()));
    TextEditorSettings::initializeEditor(newWidget);
    return newWidget->editor();
}

PlainTextDocument::PlainTextDocument()
{
    setId(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    connect(this, SIGNAL(tabSettingsChanged()), this, SLOT(updateTabSettings()));
}

void PlainTextDocument::updateTabSettings()
{
    if (Highlighter *highlighter = qobject_cast<Highlighter *>(syntaxHighlighter()))
        highlighter->setTabSettings(tabSettings());
}

} // namespace TextEditor
