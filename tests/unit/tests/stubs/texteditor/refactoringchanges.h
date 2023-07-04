// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

// #include "../utils/googletest.h"

#include <QFile>
#include <QSharedPointer>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include <utils/changeset.h>

#include <memory>

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
    {}

    const QTextDocument *document() const { return textDocument.get(); }

    void setChangeSet(const Utils::ChangeSet &changes) { this->changes = changes; }

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

    RefactoringFilePtr file(const Utils::FilePath &filePath) const
    {
        return RefactoringFilePtr(new RefactoringFile(
            std::unique_ptr<QTextDocument>(new QTextDocument(readFile(filePath.toString())))));
    }
};

class RefactoringChangesData
{
public:
    RefactoringChangesData() {}
};

} // namespace TextEditor
