/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CPPTOOLS_CPPCOMPLETIONSUPPORT_H
#define CPPTOOLS_CPPCOMPLETIONSUPPORT_H

#include "cpptools_global.h"

#include <texteditor/codeassist/assistenums.h>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
}

namespace TextEditor {
class IAssistInterface;
class ITextEditor;
}

namespace CppTools {

class CPPTOOLS_EXPORT CppCompletionSupport
{
public:
    CppCompletionSupport(TextEditor::ITextEditor *editor);
    virtual ~CppCompletionSupport() = 0;

    virtual TextEditor::IAssistInterface *createAssistInterface(ProjectExplorer::Project *project,
                                                                QTextDocument *document,
                                                                int position,
                                                                TextEditor::AssistReason reason) const = 0;

protected:
    TextEditor::ITextEditor *editor() const
    { return m_editor; }

private:
    TextEditor::ITextEditor *m_editor;
};

} // namespace CppTools

#endif // CPPTOOLS_CPPCOMPLETIONSUPPORT_H
