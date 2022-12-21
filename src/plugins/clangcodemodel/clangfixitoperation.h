// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangutils.h"

#include <texteditor/quickfix.h>

#include <utils/changeset.h>

#include <QVector>
#include <QSharedPointer>

namespace TextEditor {
class RefactoringChanges;
class RefactoringFile;
}

namespace ClangCodeModel {
namespace Internal {

class ClangFixItOperation : public TextEditor::QuickFixOperation
{
public:
    ClangFixItOperation(const QString &fixItText, const QList<ClangFixIt> &fixIts);

    int priority() const override;
    QString description() const override;
    void perform() override;

    QString firstRefactoringFileContent_forTestOnly() const;

private:
    void applyFixitsToFile(TextEditor::RefactoringFile &refactoringFile,
                           const QList<ClangFixIt> fixIts);
    Utils::ChangeSet toChangeSet(TextEditor::RefactoringFile &refactoringFile,
            const QList<ClangFixIt> fixIts) const;

private:
    QString fixItText;
    QVector<QSharedPointer<TextEditor::RefactoringFile>> refactoringFiles;
    QList<ClangFixIt> fixIts;
};

} // namespace Internal
} // namespace ClangCodeModel
