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

#include "qmljsrefactoringchanges.h"
#include "qmljsqtstylecodeformatter.h"
#include "qmljstoolsconstants.h"
#include "qmljsmodelmanager.h"
#include "qmljsindenter.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <projectexplorer/editorconfiguration.h>

using namespace QmlJS;
using namespace QmlJSTools;

class QmlJSTools::QmlJSRefactoringChangesData : public TextEditor::RefactoringChangesData
{
public:
    QmlJSRefactoringChangesData(ModelManagerInterface *modelManager,
                                const Snapshot &snapshot)
        : m_modelManager(modelManager)
        , m_snapshot(snapshot)
    {}

    virtual void indentSelection(const QTextCursor &selection,
                                 const QString &fileName,
                                 const TextEditor::BaseTextEditorWidget *textEditor) const
    {
        // ### shares code with QmlJSTextEditor::indent
        QTextDocument *doc = selection.document();

        QTextBlock block = doc->findBlock(selection.selectionStart());
        const QTextBlock end = doc->findBlock(selection.selectionEnd()).next();

        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(fileName, textEditor);
        CreatorCodeFormatter codeFormatter(tabSettings);
        codeFormatter.updateStateUntil(block);

        do {
            const int depth = codeFormatter.indentFor(block);
            if (depth != -1)
                tabSettings.indentLine(block, depth);
            codeFormatter.updateLineStateChange(block);
            block = block.next();
        } while (block.isValid() && block != end);
    }

    virtual void reindentSelection(const QTextCursor &selection,
                                   const QString &fileName,
                                   const TextEditor::BaseTextEditorWidget *textEditor) const
    {
        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(fileName, textEditor);

        QmlJSEditor::Internal::Indenter indenter;
        indenter.reindent(selection.document(), selection, tabSettings);
    }

    virtual void fileChanged(const QString &fileName)
    {
        m_modelManager->updateSourceFiles(QStringList(fileName), true);
    }

    QmlJS::ModelManagerInterface *m_modelManager;
    QmlJS::Snapshot m_snapshot;
};

QmlJSRefactoringChanges::QmlJSRefactoringChanges(ModelManagerInterface *modelManager,
                                                 const Snapshot &snapshot)
    : RefactoringChanges(new QmlJSRefactoringChangesData(modelManager, snapshot))
{
}

QmlJSRefactoringFilePtr QmlJSRefactoringChanges::file(const QString &fileName) const
{
    return QmlJSRefactoringFilePtr(new QmlJSRefactoringFile(fileName, m_data));
}

QmlJSRefactoringFilePtr QmlJSRefactoringChanges::file(
        TextEditor::BaseTextEditorWidget *editor, const Document::Ptr &document)
{
    return QmlJSRefactoringFilePtr(new QmlJSRefactoringFile(editor, document));
}

const Snapshot &QmlJSRefactoringChanges::snapshot() const
{
    return data()->m_snapshot;
}

QmlJSRefactoringChangesData *QmlJSRefactoringChanges::data() const
{
    return static_cast<QmlJSRefactoringChangesData *>(m_data.data());
}

QmlJSRefactoringFile::QmlJSRefactoringFile(const QString &fileName, const QSharedPointer<TextEditor::RefactoringChangesData> &data)
    : RefactoringFile(fileName, data)
{
    // the RefactoringFile is invalid if its not for a file with qml or js code
    if (languageOfFile(fileName) == Document::UnknownLanguage)
        m_fileName.clear();
}

QmlJSRefactoringFile::QmlJSRefactoringFile(TextEditor::BaseTextEditorWidget *editor, QmlJS::Document::Ptr document)
    : RefactoringFile(editor)
    , m_qmljsDocument(document)
{
    m_fileName = document->fileName();
}

Document::Ptr QmlJSRefactoringFile::qmljsDocument() const
{
    if (!m_qmljsDocument) {
        const QString source = document()->toPlainText();
        const QString name = fileName();
        const Snapshot &snapshot = data()->m_snapshot;

        Document::MutablePtr newDoc = snapshot.documentFromSource(source, name, languageOfFile(name));
        newDoc->parse();
        m_qmljsDocument = newDoc;
    }

    return m_qmljsDocument;
}

unsigned QmlJSRefactoringFile::startOf(const AST::SourceLocation &loc) const
{
    return position(loc.startLine, loc.startColumn);
}

bool QmlJSRefactoringFile::isCursorOn(AST::UiObjectMember *ast) const
{
    const unsigned pos = cursor().position();

    return ast->firstSourceLocation().begin() <= pos
            && pos <= ast->lastSourceLocation().end();
}

bool QmlJSRefactoringFile::isCursorOn(AST::UiQualifiedId *ast) const
{
    const unsigned pos = cursor().position();

    if (ast->identifierToken.begin() > pos)
        return false;

    AST::UiQualifiedId *last = ast;
    while (last->next)
        last = last->next;

    return pos <= ast->identifierToken.end();
}

bool QmlJSRefactoringFile::isCursorOn(AST::SourceLocation loc) const
{
    const unsigned pos = cursor().position();
    return pos >= loc.begin() && pos <= loc.end();
}

QmlJSRefactoringChangesData *QmlJSRefactoringFile::data() const
{
    return static_cast<QmlJSRefactoringChangesData *>(m_data.data());
}

void QmlJSRefactoringFile::fileChanged()
{
    m_qmljsDocument.clear();
    RefactoringFile::fileChanged();
}
