/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsrefactoringchanges.h"
#include "qmljsqtstylecodeformatter.h"
#include "qmljsmodelmanager.h"
#include "qmljsindenter.h"

#include <qmljs/parser/qmljsast_p.h>
#include <texteditor/textdocument.h>
#include <texteditor/tabsettings.h>
#include <projectexplorer/editorconfiguration.h>

using namespace QmlJS;

namespace QmlJSTools {

class QmlJSRefactoringChangesData : public TextEditor::RefactoringChangesData
{
public:
    QmlJSRefactoringChangesData(ModelManagerInterface *modelManager,
                                const Snapshot &snapshot)
        : m_modelManager(modelManager)
        , m_snapshot(snapshot)
    {}

    virtual void indentSelection(const QTextCursor &selection,
                                 const QString &fileName,
                                 const TextEditor::TextDocument *textDocument) const
    {
        // ### shares code with QmlJSTextEditor::indent
        QTextDocument *doc = selection.document();

        QTextBlock block = doc->findBlock(selection.selectionStart());
        const QTextBlock end = doc->findBlock(selection.selectionEnd()).next();

        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(fileName, textDocument);
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
                                   const TextEditor::TextDocument *textDocument) const
    {
        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(fileName, textDocument);

        QmlJSEditor::Internal::Indenter indenter;
        indenter.reindent(selection.document(), selection, tabSettings);
    }

    virtual void fileChanged(const QString &fileName)
    {
        m_modelManager->updateSourceFiles(QStringList(fileName), true);
    }

    ModelManagerInterface *m_modelManager;
    Snapshot m_snapshot;
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
        TextEditor::TextEditorWidget *editor, const Document::Ptr &document)
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
    if (ModelManagerInterface::guessLanguageOfFile(fileName) == Dialect::NoLanguage)
        m_fileName.clear();
}

QmlJSRefactoringFile::QmlJSRefactoringFile(TextEditor::TextEditorWidget *editor, Document::Ptr document)
    : RefactoringFile(editor)
    , m_qmljsDocument(document)
{
    if (document)
        m_fileName = document->fileName();
}

Document::Ptr QmlJSRefactoringFile::qmljsDocument() const
{
    if (!m_qmljsDocument) {
        const QString source = document()->toPlainText();
        const QString name = fileName();
        const Snapshot &snapshot = data()->m_snapshot;

        Document::MutablePtr newDoc = snapshot.documentFromSource(source, name,
                                                                  ModelManagerInterface::guessLanguageOfFile(name));
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

} // namespace QmlJSTools
