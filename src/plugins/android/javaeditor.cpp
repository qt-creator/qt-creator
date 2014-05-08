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

#include "javaeditor.h"
#include "javaindenter.h"
#include "javaautocompleter.h"
#include "androidconstants.h"
#include "javacompletionassistprovider.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/normalindenter.h>
#include <texteditor/highlighterutils.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>

#include <QFileInfo>

using namespace Android;
using namespace Android::Internal;

//
// JavaEditor
//

JavaEditor::JavaEditor(JavaEditorWidget *editor)
  : BaseTextEditor(editor)
{
    setContext(Core::Context(Constants::C_JAVA_EDITOR,
              TextEditor::Constants::C_TEXTEDITOR));
    setDuplicateSupported(true);
}

Core::IEditor *JavaEditor::duplicate()
{
    JavaEditorWidget *ret = new JavaEditorWidget(
                qobject_cast<JavaEditorWidget*>(editorWidget()));
    TextEditor::TextEditorSettings::initializeEditor(ret);
    return ret->editor();
}

TextEditor::CompletionAssistProvider *JavaEditor::completionAssistProvider()
{
    return ExtensionSystem::PluginManager::getObject<JavaCompletionAssistProvider>();
}

//
// JavaEditorWidget
//

JavaEditorWidget::JavaEditorWidget(QWidget *parent)
    : BaseTextEditorWidget(new JavaDocument(), parent)
{
    ctor();
}

JavaEditorWidget::JavaEditorWidget(JavaEditorWidget *other)
    : BaseTextEditorWidget(other)
{
    ctor();
}

void JavaEditorWidget::ctor()
{
    m_commentDefinition.clearCommentStyles();
    m_commentDefinition.singleLine = QLatin1String("//");
    m_commentDefinition.multiLineStart = QLatin1String("/*");
    m_commentDefinition.multiLineEnd = QLatin1String("*/");
    setAutoCompleter(new JavaAutoCompleter);
}

void JavaEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

TextEditor::BaseTextEditor *JavaEditorWidget::createEditor()
{
    return new JavaEditor(this);
}

//
// JavaDocument
//

JavaDocument::JavaDocument()
        : TextEditor::BaseTextDocument()
{
    setId(Constants::JAVA_EDITOR_ID);
    setMimeType(QLatin1String(Constants::JAVA_MIMETYPE));
    setSyntaxHighlighter(TextEditor::createGenericSyntaxHighlighter(Core::MimeDatabase::findByType(QLatin1String(Constants::JAVA_MIMETYPE))));
    setIndenter(new JavaIndenter);
}

QString JavaDocument::defaultPath() const
{
    QFileInfo fi(filePath());
    return fi.absolutePath();
}

QString JavaDocument::suggestedFileName() const
{
    QFileInfo fi(filePath());
    return fi.fileName();
}
