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
#include "cppeditorconstants.h"

#include <projectexplorer/editorconfiguration.h>

#include <utils/qtcassert.h>

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <QTextDocument>

using namespace CPlusPlus;

namespace CppEditor {

static std::unique_ptr<TextEditor::Indenter> createIndenter(const Utils::FilePath &filePath,
                                                            QTextDocument *textDocument)
{
    TextEditor::ICodeStylePreferencesFactory *factory
        = TextEditor::TextEditorSettings::codeStyleFactory(Constants::CPP_SETTINGS_ID);
    std::unique_ptr<TextEditor::Indenter> indenter(factory->createIndenter(textDocument));
    indenter->setFileName(filePath);
    return indenter;
}

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

CppRefactoringFilePtr CppRefactoringChanges::file(const Utils::FilePath &filePath) const
{
    CppRefactoringFilePtr result(new CppRefactoringFile(filePath, m_data));
    return result;
}

CppRefactoringFileConstPtr CppRefactoringChanges::fileNoEditor(const Utils::FilePath &filePath) const
{
    QTextDocument *document = nullptr;
    const QString fileName = filePath.toString();
    if (data()->m_workingCopy.contains(fileName))
        document = new QTextDocument(QString::fromUtf8(data()->m_workingCopy.source(fileName)));
    CppRefactoringFilePtr result(new CppRefactoringFile(document, filePath));
    result->m_data = m_data;

    return result;
}

const Snapshot &CppRefactoringChanges::snapshot() const
{
    return data()->m_snapshot;
}

CppRefactoringFile::CppRefactoringFile(const Utils::FilePath &filePath, const QSharedPointer<TextEditor::RefactoringChangesData> &data)
    : RefactoringFile(filePath, data)
{
    const Snapshot &snapshot = this->data()->m_snapshot;
    m_cppDocument = snapshot.document(filePath.toString());
}

CppRefactoringFile::CppRefactoringFile(QTextDocument *document, const Utils::FilePath &filePath)
    : RefactoringFile(document, filePath)
{ }

CppRefactoringFile::CppRefactoringFile(TextEditor::TextEditorWidget *editor)
    : RefactoringFile(editor)
{ }

Document::Ptr CppRefactoringFile::cppDocument() const
{
    if (!m_cppDocument || !m_cppDocument->translationUnit() ||
            !m_cppDocument->translationUnit()->ast()) {
        const QByteArray source = document()->toPlainText().toUtf8();
        const Snapshot &snapshot = data()->m_snapshot;

        m_cppDocument = snapshot.preprocessedDocument(source, filePath());
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
    int line, column;
    cppDocument()->translationUnit()->getTokenStartPosition(index, &line, &column);
    return cppDocument()->scopeAt(line, column);
}

bool CppRefactoringFile::isCursorOn(unsigned tokenIndex) const
{
    QTextCursor tc = cursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(tokenIndex);
    int end = endOf(tokenIndex);

    return cursorBegin >= start && cursorBegin <= end;
}

bool CppRefactoringFile::isCursorOn(const AST *ast) const
{
    QTextCursor tc = cursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(ast);
    int end = endOf(ast);

    return cursorBegin >= start && cursorBegin <= end;
}

Utils::ChangeSet::Range CppRefactoringFile::range(unsigned tokenIndex) const
{
    const Token &token = tokenAt(tokenIndex);
    int line, column;
    cppDocument()->translationUnit()->getPosition(token.utf16charsBegin(), &line, &column);
    const int start = document()->findBlockByNumber(line - 1).position() + column - 1;
    return {start, start + token.utf16chars()};
}

Utils::ChangeSet::Range CppRefactoringFile::range(const AST *ast) const
{
    return {startOf(ast), endOf(ast)};
}

int CppRefactoringFile::startOf(unsigned index) const
{
    int line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).utf16charsBegin(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::startOf(const AST *ast) const
{
    int firstToken = ast->firstToken();
    const int lastToken = ast->lastToken();
    while (tokenAt(firstToken).generated() && firstToken < lastToken)
        ++firstToken;
    return startOf(firstToken);
}

int CppRefactoringFile::endOf(unsigned index) const
{
    int line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).utf16charsEnd(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::endOf(const AST *ast) const
{
    int lastToken = ast->lastToken() - 1;
    QTC_ASSERT(lastToken >= 0, return -1);
    const int firstToken = ast->firstToken();
    while (tokenAt(lastToken).generated() && lastToken > firstToken)
        --lastToken;
    return endOf(lastToken);
}

void CppRefactoringFile::startAndEndOf(unsigned index, int *start, int *end) const
{
    int line, column;
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

CppRefactoringChangesData::CppRefactoringChangesData(const Snapshot &snapshot)
    : m_snapshot(snapshot)
    , m_modelManager(CppModelManager::instance())
    , m_workingCopy(m_modelManager->workingCopy())
{}

void CppRefactoringChangesData::indentSelection(const QTextCursor &selection,
                                                const Utils::FilePath &filePath,
                                                const TextEditor::TextDocument *textDocument) const
{
    if (textDocument) { // use the indenter from the textDocument if there is one, can be ClangFormat
        textDocument->indenter()->indent(selection, QChar::Null, textDocument->tabSettings());
    } else {
        const auto &tabSettings = ProjectExplorer::actualTabSettings(filePath, textDocument);
        auto indenter = createIndenter(filePath, selection.document());
        indenter->indent(selection, QChar::Null, tabSettings);
    }
}

void CppRefactoringChangesData::reindentSelection(const QTextCursor &selection,
                                                  const Utils::FilePath &filePath,
                                                  const TextEditor::TextDocument *textDocument) const
{
    if (textDocument) { // use the indenter from the textDocument if there is one, can be ClangFormat
        textDocument->indenter()->reindent(selection, textDocument->tabSettings());
    } else {
        const auto &tabSettings = ProjectExplorer::actualTabSettings(filePath, textDocument);
        auto indenter = createIndenter(filePath, selection.document());
        indenter->reindent(selection, tabSettings);
    }
}

void CppRefactoringChangesData::fileChanged(const Utils::FilePath &filePath)
{
    m_modelManager->updateSourceFiles({filePath.toString()});
}

} // namespace CppEditor
