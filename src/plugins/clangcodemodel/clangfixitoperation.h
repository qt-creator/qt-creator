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

#ifndef CLANGCODEMODEL_CLANGFIXITOPERATION_H
#define CLANGCODEMODEL_CLANGFIXITOPERATION_H

#include <texteditor/quickfix.h>

#include <clangbackendipc/fixitcontainer.h>

#include <utils/changeset.h>

#include <QVector>
#include <QSharedPointer>

namespace TextEditor
{
class RefactoringFile;
}

namespace ClangCodeModel {

class ClangFixItOperation : public TextEditor::QuickFixOperation
{
public:
    ClangFixItOperation(const Utf8String &filePath,
                        const Utf8String &fixItText,
                        const QVector<ClangBackEnd::FixItContainer> &fixItContainers);

    int priority() const override;
    QString description() const override;
    void perform() override;

    QString refactoringFileContent_forTestOnly() const;

private:
    Utils::ChangeSet changeSet() const;

private:
    Utf8String filePath;
    Utf8String fixItText;
    QSharedPointer<TextEditor::RefactoringFile> refactoringFile;
    QVector<ClangBackEnd::FixItContainer> fixItContainers;
};

} // namespace ClangCodeModel

#endif // CLANGCODEMODEL_CLANGFIXITOPERATION_H
