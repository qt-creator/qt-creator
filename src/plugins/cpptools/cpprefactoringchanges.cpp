/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "cpprefactoringchanges.h"
#include "cppcodestylepreferences.h"

#include <TranslationUnit.h>
#include <AST.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolssettings.h>
#include <cpptools/cpptoolsconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <projectexplorer/editorconfiguration.h>

#include <QtGui/QTextBlock>

using namespace CPlusPlus;
using namespace CppTools;
using namespace Utils;

CppRefactoringChanges::CppRefactoringChanges(const Snapshot &snapshot)
    : m_snapshot(snapshot)
    , m_modelManager(Internal::CppModelManager::instance())
{
    Q_ASSERT(m_modelManager);
    m_workingCopy = m_modelManager->workingCopy();
}

const Snapshot &CppRefactoringChanges::snapshot() const
{
    return m_snapshot;
}

CppRefactoringFile CppRefactoringChanges::file(const QString &fileName)
{
    return CppRefactoringFile(fileName, this);
}

void CppRefactoringChanges::indentSelection(const QTextCursor &selection,
                                            const QString &fileName,
                                            const TextEditor::BaseTextEditorWidget *textEditor) const
{
    // ### shares code with CPPEditor::indent()
    QTextDocument *doc = selection.document();

    QTextBlock block = doc->findBlock(selection.selectionStart());
    const QTextBlock end = doc->findBlock(selection.selectionEnd()).next();

    const TextEditor::TabSettings &tabSettings =
        ProjectExplorer::actualTabSettings(fileName, textEditor);
    // TODO: add similar method like above one
    CppTools::QtStyleCodeFormatter codeFormatter(tabSettings,
        CppToolsSettings::instance()->cppCodeStylePreferences()->settings());
    codeFormatter.updateStateUntil(block);

    do {
        int indent;
        int padding;
        codeFormatter.indentFor(block, &indent, &padding);
        tabSettings.indentLine(block, indent + padding, padding);
        codeFormatter.updateLineStateChange(block);
        block = block.next();
    } while (block.isValid() && block != end);
}

void CppRefactoringChanges::fileChanged(const QString &fileName)
{
    m_modelManager->updateSourceFiles(QStringList(fileName));
}


CppRefactoringFile::CppRefactoringFile()
{ }

CppRefactoringFile::CppRefactoringFile(const QString &fileName, CppRefactoringChanges *refactoringChanges)
    : RefactoringFile(fileName, refactoringChanges)
{
    const Snapshot &snapshot = refactoringChanges->snapshot();
    m_cppDocument = snapshot.document(fileName);
}

CppRefactoringFile::CppRefactoringFile(TextEditor::BaseTextEditorWidget *editor, CPlusPlus::Document::Ptr document)
    : RefactoringFile()
    , m_cppDocument(document)
{
    m_fileName = document->fileName();
    m_editor = editor;
}

Document::Ptr CppRefactoringFile::cppDocument() const
{
    if (!m_cppDocument || !m_cppDocument->translationUnit() ||
            !m_cppDocument->translationUnit()->ast()) {
        const QString source = document()->toPlainText();
        const QString name = fileName();
        const Snapshot &snapshot = refactoringChanges()->snapshot();

        const QByteArray contents = snapshot.preprocessedCode(source, name);
        m_cppDocument = snapshot.documentFromSource(contents, name);
        m_cppDocument->check();
    }

    return m_cppDocument;
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

ChangeSet::Range CppRefactoringFile::range(unsigned tokenIndex) const
{
    const Token &token = tokenAt(tokenIndex);
    unsigned line, column;
    cppDocument()->translationUnit()->getPosition(token.begin(), &line, &column);
    const int start = document()->findBlockByNumber(line - 1).position() + column - 1;
    return ChangeSet::Range(start, start + token.length());
}

ChangeSet::Range CppRefactoringFile::range(AST *ast) const
{
    return ChangeSet::Range(startOf(ast), endOf(ast));
}

int CppRefactoringFile::startOf(unsigned index) const
{
    unsigned line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).begin(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::startOf(const AST *ast) const
{
    return startOf(ast->firstToken());
}

int CppRefactoringFile::endOf(unsigned index) const
{
    unsigned line, column;
    cppDocument()->translationUnit()->getPosition(tokenAt(index).end(), &line, &column);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppRefactoringFile::endOf(const AST *ast) const
{
    unsigned end = ast->lastToken();
    Q_ASSERT(end > 0);
    return endOf(end - 1);
}

void CppRefactoringFile::startAndEndOf(unsigned index, int *start, int *end) const
{
    unsigned line, column;
    Token token(tokenAt(index));
    cppDocument()->translationUnit()->getPosition(token.begin(), &line, &column);
    *start = document()->findBlockByNumber(line - 1).position() + column - 1;
    *end = *start + token.length();
}

QString CppRefactoringFile::textOf(const AST *ast) const
{
    return textOf(startOf(ast), endOf(ast));
}

const Token &CppRefactoringFile::tokenAt(unsigned index) const
{
    return cppDocument()->translationUnit()->tokenAt(index);
}

CppRefactoringChanges *CppRefactoringFile::refactoringChanges() const
{
    return static_cast<CppRefactoringChanges *>(m_refactoringChanges);
}
