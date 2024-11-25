// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "indenter.h"

#include <texteditor/texteditor_global.h>

#include <utils/changeset.h>
#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/textfileformat.h>

#include <QList>
#include <QPair>
#include <QSharedPointer>
#include <QString>

#include <utility>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {
class PlainRefactoringFileFactory;
class RefactoringFile;
using RefactoringFilePtr = QSharedPointer<RefactoringFile>;
using RefactoringSelections = QVector<QPair<QTextCursor, QTextCursor>>;
class TextDocument;
class TextEditorWidget;

// ### listen to the m_editor::destroyed signal?
class TEXTEDITOR_EXPORT RefactoringFile
{
    Q_DISABLE_COPY(RefactoringFile)
    friend class PlainRefactoringFileFactory; // access to constructor
public:
    using Range = Utils::ChangeSet::Range;

    virtual ~RefactoringFile();

    bool isValid() const;

    const QTextDocument *document() const;
    const QTextCursor cursor() const; // mustn't use the cursor to change the document
    Utils::FilePath filePath() const;
    TextEditorWidget *editor() const;

    // converts 1-based line and column into 0-based source offset
    int position(int line, int column) const;
    // converts 0-based source offset into 1-based line and column
    void lineAndColumn(int offset, int *line, int *column) const;

    QChar charAt(int pos) const;
    QString textOf(int start, int end) const;
    QString textOf(const Range &range) const;

    Utils::ChangeSet changeSet() const;
    void setChangeSet(const Utils::ChangeSet &changeSet);
    void setOpenEditor(bool activate = false, int pos = -1);
    bool apply();
    bool apply(const Utils::ChangeSet &changeSet);
    bool create(const QString &contents, bool reindent, bool openInEditor);

protected:
    // users may only get const access to RefactoringFiles created through
    // this constructor, because it can't be used to apply changes
    RefactoringFile(QTextDocument *document, const Utils::FilePath &filePath);

    RefactoringFile(TextEditorWidget *editor);
    RefactoringFile(const Utils::FilePath &filePath);

    void invalidate() { m_filePath.clear(); }

private:
    virtual void fileChanged() {} // derived classes may want to clear language specific extra data
    virtual Utils::Id indenterId() const { return {} ;}

    void setupFormattingRanges(const QList<Utils::ChangeSet::EditOp> &replaceList);
    void doFormatting();

    TextEditorWidget *openEditor(bool activate, int line, int column);
    QTextDocument *mutableDocument() const;

    Utils::FilePath m_filePath;
    mutable Utils::TextFileFormat m_textFileFormat;
    mutable QTextDocument *m_document = nullptr;
    TextEditorWidget *m_editor = nullptr;
    Utils::ChangeSet m_changes;

    // The bool indicates whether to advance the start position.
    QList<std::pair<QTextCursor, bool>> m_formattingCursors;

    bool m_openEditor = false;
    bool m_activateEditor = false;
    int m_editorCursorPosition = -1;
    bool m_appliedOnce = false;
};

class TEXTEDITOR_EXPORT RefactoringFileFactory
{
public:
    virtual ~RefactoringFileFactory();
    virtual RefactoringFilePtr file(const Utils::FilePath &filePath) const = 0;
};

class TEXTEDITOR_EXPORT PlainRefactoringFileFactory final : public RefactoringFileFactory
{
public:
    RefactoringFilePtr file(const Utils::FilePath &filePath) const override;
};

} // namespace TextEditor
