/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

