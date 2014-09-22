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

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <utils/uncommentselection.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/normalindenter.h>
#include <extensionsystem/pluginmanager.h>

#include <QFileInfo>

namespace Android {
namespace Internal {

//
// JavaEditorWidget
//

class JavaEditorWidget : public TextEditor::BaseTextEditorWidget
{
public:
    JavaEditorWidget()
    {
        setCompletionAssistProvider(ExtensionSystem::PluginManager::getObject<JavaCompletionAssistProvider>());
    }
};


//
// JavaDocument
//

class JavaDocument : public TextEditor::TextDocument
{
public:
    JavaDocument();
    QString defaultPath() const;
    QString suggestedFileName() const;
};


JavaDocument::JavaDocument()
{
    setId(Constants::JAVA_EDITOR_ID);
    setMimeType(QLatin1String(Constants::JAVA_MIMETYPE));
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


//
// JavaEditorFactory
//

JavaEditorFactory::JavaEditorFactory()
{
    setId(Constants::JAVA_EDITOR_ID);
    setDisplayName(tr("Java Editor"));
    addMimeType(Constants::JAVA_MIMETYPE);

    setDocumentCreator([]() { return new JavaDocument; });
    setEditorWidgetCreator([]() { return new JavaEditorWidget; });
    setAutoCompleterCreator([]() { return new JavaAutoCompleter; });
    setGenericSyntaxHighlighter(QLatin1String(Constants::JAVA_MIMETYPE));
    setCommentStyle(Utils::CommentDefinition::CppStyle);

    setEditorActionHandlers(TextEditor::TextEditorActionHandler::UnCommentSelection);
}

} // namespace Internal
} // namespace Android
