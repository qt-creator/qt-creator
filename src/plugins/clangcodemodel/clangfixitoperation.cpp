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

#include "clangfixitoperation.h"

#include <texteditor/refactoringchanges.h>

#include <QTextDocument>

namespace ClangCodeModel {

ClangFixItOperation::ClangFixItOperation(const Utf8String &filePath,
                                         const Utf8String &fixItText,
                                         const QVector<ClangBackEnd::FixItContainer> &fixItContainers)
    : filePath(filePath),
      fixItText(fixItText),
      fixItContainers(fixItContainers)
{
}

int ClangFixItOperation::priority() const
{
    return 10;
}

QString ClangCodeModel::ClangFixItOperation::description() const
{
    return QStringLiteral("Apply Fix: ") + fixItText.toString();
}

void ClangFixItOperation::perform()
{
    const TextEditor::RefactoringChanges refactoringChanges;
    refactoringFile = refactoringChanges.file(filePath.toString());
    refactoringFile->setChangeSet(changeSet());
    refactoringFile->apply();
}

QString ClangFixItOperation::refactoringFileContent_forTestOnly() const
{
    return refactoringFile->document()->toPlainText();
}

Utils::ChangeSet ClangFixItOperation::changeSet() const
{
    Utils::ChangeSet changeSet;

    for (const auto &fixItContainer : fixItContainers) {
        const auto range = fixItContainer.range();
        const auto start = range.start();
        const auto end = range.end();
        changeSet.replace(refactoringFile->position(start.line(), start.column()),
                          refactoringFile->position(end.line(), end.column()),
                          fixItContainer.text());
    }

    return changeSet;
}

} // namespace ClangCodeModel

