// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    void indentSelection(const QTextCursor &selection,
                         const Utils::FilePath &filePath,
                         const TextEditor::TextDocument *textDocument) const override
    {
        // ### shares code with QmlJSTextEditor::indent
        QTextDocument *doc = selection.document();

        QTextBlock block = doc->findBlock(selection.selectionStart());
        const QTextBlock end = doc->findBlock(selection.selectionEnd()).next();

        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(filePath, textDocument);
        CreatorCodeFormatter codeFormatter(tabSettings);
        codeFormatter.updateStateUntil(block);
        do {
            int depth = codeFormatter.indentFor(block);
            if (depth != -1) {
                if (QStringView(block.text()).trimmed().isEmpty()) {
                    // we do not want to indent empty lines (as one is indentent when pressing tab
                    // assuming that the user will start writing something), and get rid of that
                    // space if one had pressed tab in an empty line just before refactoring.
                    // If depth == -1 (inside a multiline string for example) leave the spaces.
                    depth = 0;
                }
                tabSettings.indentLine(block, depth);
            }
            codeFormatter.updateLineStateChange(block);
            block = block.next();
        } while (block.isValid() && block != end);
    }

    void reindentSelection(const QTextCursor &selection,
                           const Utils::FilePath &filePath,
                           const TextEditor::TextDocument *textDocument) const override
    {
        const TextEditor::TabSettings &tabSettings =
            ProjectExplorer::actualTabSettings(filePath, textDocument);

        QmlJSEditor::Internal::Indenter indenter(selection.document());
        indenter.reindent(selection, tabSettings);
    }

    void fileChanged(const Utils::FilePath &filePath) override
    {
        m_modelManager->updateSourceFiles({filePath}, true);
    }

    ModelManagerInterface *m_modelManager;
    Snapshot m_snapshot;
};

QmlJSRefactoringChanges::QmlJSRefactoringChanges(ModelManagerInterface *modelManager,
                                                 const Snapshot &snapshot)
    : RefactoringChanges(new QmlJSRefactoringChangesData(modelManager, snapshot))
{
}

QmlJSRefactoringFilePtr QmlJSRefactoringChanges::file(const Utils::FilePath &filePath) const
{
    return QmlJSRefactoringFilePtr(new QmlJSRefactoringFile(filePath, m_data));
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

QmlJSRefactoringFile::QmlJSRefactoringFile(
    const Utils::FilePath &filePath, const QSharedPointer<TextEditor::RefactoringChangesData> &data)
    : RefactoringFile(filePath, data)
{
    // the RefactoringFile is invalid if its not for a file with qml or js code
    if (ModelManagerInterface::guessLanguageOfFile(filePath) == Dialect::NoLanguage)
        m_filePath.clear();
}

QmlJSRefactoringFile::QmlJSRefactoringFile(TextEditor::TextEditorWidget *editor, Document::Ptr document)
    : RefactoringFile(editor)
    , m_qmljsDocument(document)
{
    if (document)
        m_filePath = document->fileName();
}

Document::Ptr QmlJSRefactoringFile::qmljsDocument() const
{
    if (!m_qmljsDocument) {
        const QString source = document()->toPlainText();
        const Snapshot &snapshot = data()->m_snapshot;

        Document::MutablePtr newDoc
            = snapshot.documentFromSource(source,
                                          filePath(),
                                          ModelManagerInterface::guessLanguageOfFile(filePath()));
        newDoc->parse();
        m_qmljsDocument = newDoc;
    }

    return m_qmljsDocument;
}

QString QmlJSRefactoringFile::qmlImports() const
{
    QString imports;
    QmlJS::AST::UiProgram *prog = qmljsDocument()->qmlProgram();
    if (prog && prog->headers) {
        const unsigned int start = startOf(prog->headers->firstSourceLocation());
        const unsigned int end = startOf(prog->members->member->firstSourceLocation());
        imports = textOf(start, end);
    }
    return imports;
}

unsigned QmlJSRefactoringFile::startOf(const SourceLocation &loc) const
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

bool QmlJSRefactoringFile::isCursorOn(SourceLocation loc) const
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
