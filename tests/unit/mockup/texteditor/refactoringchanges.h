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

#pragma once

#include <QFile>
#include <QSharedPointer>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include <utils/changeset.h>

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "gtest-qt-printing.h"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Utils {
class ChangeSet;
}

namespace TextEditor {
class RefactoringChanges;
class RefactoringFile;
class RefactoringChangesData;
typedef QSharedPointer<RefactoringFile> RefactoringFilePtr;

using testing::NotNull;

class RefactoringFile
{
public:
    RefactoringFile(std::unique_ptr<QTextDocument> &&textDocument)
        : textDocument(std::move(textDocument))
    {
    }

    const QTextDocument *document() const
    {
        return textDocument.get();
    }

    void setChangeSet(const Utils::ChangeSet &changes)
    {
        this->changes = changes;
    }

    void apply()
    {
        QTextCursor textCursor(textDocument.get());
        changes.apply(&textCursor);
        changes.clear();
    }

    int position(uint line, uint column)
    {
        return textDocument->findBlockByNumber(uint(line) - 1).position() + int(column) - 1;
    }

private:
    std::unique_ptr<QTextDocument> textDocument;
    Utils::ChangeSet changes;
};

QString readFile(const QString &filePath)
{
    EXPECT_FALSE(filePath.isEmpty());

    QFile file(filePath);

    EXPECT_TRUE(file.open(QFile::ReadOnly));

    auto content = file.readAll();

    EXPECT_FALSE(content.isEmpty());

    return QString::fromUtf8(content);
}

class RefactoringChanges
{
public:
    RefactoringChanges() {}
    virtual ~RefactoringChanges() {}

    RefactoringFilePtr file(const QString &filePath) const
    {
        return RefactoringFilePtr(new RefactoringFile(std::unique_ptr<QTextDocument>(new QTextDocument(readFile(filePath)))));
    }
};

class RefactoringChangesData
{
public:
    RefactoringChangesData() {}
};

} // namespace TextEditor
