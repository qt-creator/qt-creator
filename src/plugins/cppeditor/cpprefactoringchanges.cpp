// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpprefactoringchanges.h"

#include "cppeditorconstants.h"
#include "cppeditorwidget.h"
#include "cppsemanticinfo.h"
#include "cppworkingcopy.h"

#include <projectexplorer/editorconfiguration.h>

#include <utils/qtcassert.h>

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <QTextDocument>

#include <utility>

using namespace Core;
using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {
namespace Internal {
class CppRefactoringChangesData
{
public:
    explicit CppRefactoringChangesData(const CPlusPlus::Snapshot &snapshot);

    CPlusPlus::Snapshot m_snapshot;
    WorkingCopy m_workingCopy;
};
} // namespace Internal

using namespace Internal;

CppRefactoringChanges::CppRefactoringChanges(const Snapshot &snapshot)
    : m_data(new CppRefactoringChangesData(snapshot))
{
}

CppRefactoringFilePtr CppRefactoringChanges::file(
    TextEditor::TextEditorWidget *editor, const Document::Ptr &document)
{
    CppRefactoringFilePtr result(new CppRefactoringFile(editor));
    result->setCppDocument(document);
    if (const auto cppEditorWidget = qobject_cast<CppEditorWidget *>(editor)) {
        result->m_data = QSharedPointer<CppRefactoringChangesData>::create(
            cppEditorWidget->semanticInfo().snapshot);
    }
    return result;
}

TextEditor::RefactoringFilePtr CppRefactoringChanges::file(const FilePath &filePath) const
{
    return cppFile(filePath);
}

CppRefactoringFilePtr CppRefactoringChanges::cppFile(const Utils::FilePath &filePath) const
{
    // Prefer documents from editors, as these are already parsed and up to date with regards to
    // unsaved changes.
    const QList<IEditor *> editors = DocumentModel::editorsForFilePath(filePath);
    for (IEditor *editor : editors) {
        if (const auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor)) {
            if (const auto editorWidget = qobject_cast<CppEditorWidget *>(
                    textEditor->editorWidget())) {
                return file(editorWidget, editorWidget->semanticInfo().doc);
            }
        }
    }

    return CppRefactoringFilePtr(new CppRefactoringFile(filePath, m_data));
}

CppRefactoringFileConstPtr CppRefactoringChanges::fileNoEditor(const FilePath &filePath) const
{
    QTextDocument *document = nullptr;
    if (const auto source = m_data->m_workingCopy.source(filePath))
        document = new QTextDocument(QString::fromUtf8(*source));
    CppRefactoringFilePtr result(new CppRefactoringFile(document, filePath));
    result->m_data = m_data.staticCast<CppRefactoringChangesData>();

    return result;
}

const Snapshot &CppRefactoringChanges::snapshot() const
{
    return m_data->m_snapshot;
}

CppRefactoringFile::CppRefactoringFile(const FilePath &filePath, const QSharedPointer<CppRefactoringChangesData> &data)
    : RefactoringFile(filePath), m_data(data)
{
    const Snapshot &snapshot = data->m_snapshot;
    m_cppDocument = snapshot.document(filePath);
}

CppRefactoringFile::CppRefactoringFile(QTextDocument *document, const FilePath &filePath)
    : RefactoringFile(document, filePath)
{
}

CppRefactoringFile::CppRefactoringFile(TextEditor::TextEditorWidget *editor)
    : RefactoringFile(editor)
{
}

Document::Ptr CppRefactoringFile::cppDocument() const
{
    if (!m_cppDocument || !m_cppDocument->translationUnit() ||
            !m_cppDocument->translationUnit()->ast()) {
        const QByteArray source = document()->toPlainText().toUtf8();
        const Snapshot &snapshot = m_data->m_snapshot;

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
    cppDocument()->translationUnit()->getTokenPosition(index, &line, &column);
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
    if (!ast)
        return false;

    QTextCursor tc = cursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(ast);
    int end = endOf(ast);

    return cursorBegin >= start && cursorBegin <= end;
}

QList<Token> CppRefactoringFile::tokensForCursor() const
{
    return tokensForCursor(cursor());
}

QList<Token> CppRefactoringFile::tokensForCursor(const QTextCursor &cursor) const
{
    int pos = cursor.selectionStart();
    int endPos = cursor.selectionEnd();
    if (pos > endPos)
        std::swap(pos, endPos);

    // Skip whitespace.
    while (pos < endPos && document()->characterAt(pos).isSpace())
        ++pos;
    while (endPos > pos && document()->characterAt(endPos).isSpace())
        --endPos;

    const std::vector<Token> &allTokens = cppDocument()->translationUnit()->allTokens();
    const int firstIndex = tokenIndexForPosition(allTokens, pos, 0);
    if (firstIndex == -1)
        return {};

    const int lastIndex = pos == endPos
                              ? firstIndex
                              : tokenIndexForPosition(allTokens, endPos, firstIndex);
    if (lastIndex == -1)
        return {};
    QTC_ASSERT(lastIndex >= firstIndex, return {});
    QList<Token> result;
    for (int i = firstIndex; i <= lastIndex; ++i)
        result.push_back(allTokens.at(i));
    return result;
}

QList<Token> CppRefactoringFile::tokensForLine(int line) const
{
    const QTextBlock block = document()->findBlockByNumber(line - 1);
    QTextCursor cursor(block);
    cursor.select(QTextCursor::BlockUnderCursor);
    return tokensForCursor(cursor);
}

ChangeSet::Range CppRefactoringFile::range(unsigned tokenIndex) const
{
    const Token &token = tokenAt(tokenIndex);
    int line, column;
    cppDocument()->translationUnit()->getPosition(token.utf16charsBegin(), &line, &column);
    const int start = document()->findBlockByNumber(line - 1).position() + column - 1;
    return {start, start + token.utf16chars()};
}

ChangeSet::Range CppRefactoringFile::range(const AST *ast) const
{
    return {startOf(ast), endOf(ast)};
}

int CppRefactoringFile::startOf(unsigned index) const
{
    if (const auto loc = expansionLoc(index))
        return loc->first;

    int line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).utf16charsBegin(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::startOf(const AST *ast) const
{
    QTC_ASSERT(ast, return 0);
    return startOf(ast->firstToken());
}

int CppRefactoringFile::endOf(unsigned index) const
{
    if (const auto loc = expansionLoc(index))
        return loc->first + loc->second;

    int line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).utf16charsEnd(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::endOf(const AST *ast) const
{
    QTC_ASSERT(ast, return 0);
    int lastToken = ast->lastToken() - 1;
    QTC_ASSERT(lastToken >= 0, return -1);
    return endOf(lastToken);
}

void CppRefactoringFile::startAndEndOf(unsigned index, int *start, int *end) const
{
    if (const auto loc = expansionLoc(index)) {
        *start = loc->first;
        *end = loc->first + loc->second;
        return;
    }

    int line, column;
    Token token(tokenAt(index));
    cppDocument()->translationUnit()->getPosition(token.utf16charsBegin(), &line, &column);
    *start = document()->findBlockByNumber(line - 1).position() + column - 1;
    *end = *start + token.utf16chars();
}

std::optional<std::pair<int, int> > CppRefactoringFile::expansionLoc(unsigned int index) const
{
    if (!tokenAt(index).expanded())
        return {};
    return cppDocument()->translationUnit()->getExpansionPosition(index);
}

QString CppRefactoringFile::textOf(const AST *ast) const
{
    return textOf(startOf(ast), endOf(ast));
}

const Token &CppRefactoringFile::tokenAt(unsigned index) const
{
    return cppDocument()->translationUnit()->tokenAt(index);
}

void CppRefactoringFile::fileChanged()
{
    QTC_ASSERT(!filePath().isEmpty(), return);
    m_cppDocument.clear();
    CppModelManager::updateSourceFiles({filePath()});
}

Id CppRefactoringFile::indenterId() const
{
    return Constants::CPP_SETTINGS_ID;
}

int CppRefactoringFile::tokenIndexForPosition(const std::vector<CPlusPlus::Token> &tokens,
                                              int pos, int startIndex) const
{
    const TranslationUnit * const tu = cppDocument()->translationUnit();

    // Binary search
    for (int l = startIndex, u = int(tokens.size()) - 1; l <= u; ) {
        const int i = (l + u) / 2;
        const int tokenPos = tu->getTokenPositionInDocument(tokens.at(i), document());
        if (pos < tokenPos) {
            u = i - 1;
            continue;
        }
        const int tokenEndPos = tu->getTokenEndPositionInDocument(tokens.at(i), document());
        if (pos > tokenEndPos) {
            l = i + 1;
            continue;
        }
        return i;
    }
    return -1;
}

CppRefactoringChangesData::CppRefactoringChangesData(const Snapshot &snapshot)
    : m_snapshot(snapshot)
    , m_workingCopy(CppModelManager::workingCopy())
{}

} // namespace CppEditor
