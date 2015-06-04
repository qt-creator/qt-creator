/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "javaeditor.h"
#include "javaindenter.h"
#include "javaautocompleter.h"
#include "androidconstants.h"
#include "javacompletionassistprovider.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <texteditor/normalindenter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditor.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/fileutils.h>
#include <utils/uncommentselection.h>

#include <QFileInfo>

namespace Android {
namespace Internal {

//
// JavaDocument
//

class JavaDocument : public TextEditor::TextDocument
{
public:
    JavaDocument();
    QString defaultPath() const override;
    QString suggestedFileName() const override;
};


JavaDocument::JavaDocument()
{
    setId(Constants::JAVA_EDITOR_ID);
    setMimeType(QLatin1String(Constants::JAVA_MIMETYPE));
    setIndenter(new JavaIndenter);
}

QString JavaDocument::defaultPath() const
{
    return filePath().toFileInfo().absolutePath();
}

QString JavaDocument::suggestedFileName() const
{
    return filePath().fileName();
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
    setAutoCompleterCreator([]() { return new JavaAutoCompleter; });
    setUseGenericHighlighter(true);
    setCommentStyle(Utils::CommentDefinition::CppStyle);
    setEditorActionHandlers(TextEditor::TextEditorActionHandler::UnCommentSelection);
    setCompletionAssistProvider(new JavaCompletionAssistProvider);
    setMarksVisible(true);
}

} // namespace Internal
} // namespace Android
