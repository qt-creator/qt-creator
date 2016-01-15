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

#include "cpprefactoringchanges.h"

#include "cppqtstyleindenter.h"
#include "cppcodeformatter.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"

#include <projectexplorer/editorconfiguration.h>

#include <utils/qtcassert.h>

#include <QTextDocument>

using namespace CPlusPlus;

namespace CppTools {

class CppRefactoringChangesData : public TextEditor::RefactoringChangesData
{
public:
    CppRefactoringChangesData(const Snapshot &snapshot)
        : m_snapshot(snapshot)
        , m_modelManager(CppModelManager::instance())
        , m_workingCopy(m_modelManager->workingCopy())
    {}

    virtual void indentSelection(const QTextCursor &selection,
                                 const QString &fileName,
                                 const TextEditor::TextDocument *textDocument) const
    {
        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(fileName, textDocument);

        CppQtStyleIndenter indenter;
        indenter.indent(selection.document(), selection, QChar::Null, tabSettings);
    }

    virtual void reindentSelection(const QTextCursor &selection,
                                   const QString &fileName,
                                   const TextEditor::TextDocument *textDocument) const
    {
        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(fileName, textDocument);

        CppQtStyleIndenter indenter;
        indenter.reindent(selection.document(), selection, tabSettings);
    }

    virtual void fileChanged(const QString &fileName)
    {
        m_modelManager->updateSourceFiles(QSet<QString>() << fileName);
    }

    Snapshot m_snapshot;
    CppModelManager *m_modelManager;
    WorkingCopy m_workingCopy;

};

CppRefactoringChanges::CppRefactoringChanges(const Snapshot &snapshot)
    : RefactoringChanges(new CppRefactoringChangesData(snapshot))
{
}

CppRefactoringChangesData *CppRefactoringChanges::data() const
{
    return static_cast<CppRefactoringChangesData *>(m_data.data());
}

CppRefactoringFilePtr CppRefactoringChanges::file(TextEditor::TextEditorWidget *editor, const Document::Ptr &document)
{
    CppRefactoringFilePtr result(new CppRefactoringFile(editor));
    result->setCppDocument(document);
    return result;
}

CppRefactoringFilePtr CppRefactoringChanges::file(const QString &fileName) const
{
    CppRefactoringFilePtr result(new CppRefactoringFile(fileName, m_data));
    return result;
}

CppRefactoringFileConstPtr CppRefactoringChanges::fileNoEditor(const QString &fileName) const
{
    QTextDocument *document = 0;
    if (data()->m_workingCopy.contains(fileName))
        document = new QTextDocument(QString::fromUtf8(data()->m_workingCopy.source(fileName)));
    CppRefactoringFilePtr result(new CppRefactoringFile(document, fileName));
    result->m_data = m_data;

    return result;
}

const Snapshot &CppRefactoringChanges::snapshot() const
{
    return data()->m_snapshot;
}

CppRefactoringFile::CppRefactoringFile(const QString &fileName, const QSharedPointer<TextEditor::RefactoringChangesData> &data)
    : RefactoringFile(fileName, data)
{
    const Snapshot &snapshot = this->data()->m_snapshot;
    m_cppDocument = snapshot.document(fileName);
}

CppRefactoringFile::CppRefactoringFile(QTextDocument *document, const QString &fileName)
    : RefactoringFile(document, fileName)
{ }

CppRefactoringFile::CppRefactoringFile(TextEditor::TextEditorWidget *editor)
    : RefactoringFile(editor)
{ }

Document::Ptr CppRefactoringFile::cppDocument() const
{
    if (!m_cppDocument || !m_cppDocument->translationUnit() ||
            !m_cppDocument->translationUnit()->ast()) {
        const QByteArray source = document()->toPlainText().toUtf8();
        const QString name = fileName();
        const Snapshot &snapshot = data()->m_snapshot;

        m_cppDocument = snapshot.preprocessedDocument(source, name);
        m_cppDocument->check();
    }

    return m_cppDocument;
}

void CppRefactoringFile::setCppDocument(Document::Ptr document)
{
    m_cppDocument = document;
}

Scope *CppRefactoringFile::scopeAt(unsigned index) const
{
    unsigned line, column;
    cppDocument()->translationUnit()->getTokenStartPosition(index, &line, &column);
    return cppDocument()->scopeAt(line, column);
}

bool CppRefactoringFile::isCursorOn(unsigned tokenIndex) const
{
    QTextCursor tc = cursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(tokenIndex);
    int end = endOf(tokenIndex);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

bool CppRefactoringFile::isCursorOn(const AST *ast) const
{
    QTextCursor tc = cursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(ast);
    int end = endOf(ast);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

Utils::ChangeSet::Range CppRefactoringFile::range(unsigned tokenIndex) const
{
    const Token &token = tokenAt(tokenIndex);
    unsigned line, column;
    cppDocument()->translationUnit()->getPosition(token.utf16charsBegin(), &line, &column);
    const int start = document()->findBlockByNumber(line - 1).position() + column - 1;
    return Utils::ChangeSet::Range(start, start + token.utf16chars());
}

Utils::ChangeSet::Range CppRefactoringFile::range(AST *ast) const
{
    return Utils::ChangeSet::Range(startOf(ast), endOf(ast));
}

int CppRefactoringFile::startOf(unsigned index) const
{
    unsigned line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).utf16charsBegin(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::startOf(const AST *ast) const
{
    return startOf(ast->firstToken());
}

int CppRefactoringFile::endOf(unsigned index) const
{
    unsigned line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).utf16charsEnd(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::endOf(const AST *ast) const
{
    unsigned end = ast->lastToken();
    QTC_ASSERT(end > 0, return -1);
    return endOf(end - 1);
}

void CppRefactoringFile::startAndEndOf(unsigned index, int *start, int *end) const
{
    unsigned line, column;
    Token token(tokenAt(index));
    cppDocument()->translationUnit()->getPosition(token.utf16charsBegin(), &line, &column);
    *start = document()->findBlockByNumber(line - 1).position() + column - 1;
    *end = *start + token.utf16chars();
}

QString CppRefactoringFile::textOf(const AST *ast) const
{
    return textOf(startOf(ast), endOf(ast));
}

const Token &CppRefactoringFile::tokenAt(unsigned index) const
{
    return cppDocument()->translationUnit()->tokenAt(index);
}

CppRefactoringChangesData *CppRefactoringFile::data() const
{
    return static_cast<CppRefactoringChangesData *>(m_data.data());
}

void CppRefactoringFile::fileChanged()
{
    m_cppDocument.clear();
    RefactoringFile::fileChanged();
}

} // namespace CppTools
