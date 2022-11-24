// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpptodoitemsscanner.h"

#include <cppeditor/projectinfo.h>

#include <projectexplorer/project.h>

#include <cplusplus/TranslationUnit.h>

#include <utils/algorithm.h>

#include <cctype>

using namespace Utils;

namespace Todo {
namespace Internal {

CppTodoItemsScanner::CppTodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    TodoItemsScanner(keywordList, parent)
{
    CppEditor::CppModelManager *modelManager = CppEditor::CppModelManager::instance();

    connect(modelManager, &CppEditor::CppModelManager::documentUpdated,
            this, &CppTodoItemsScanner::documentUpdated, Qt::DirectConnection);

    setParams(keywordList);
}

void CppTodoItemsScanner::scannerParamsChanged()
{
    // We need to rescan everything known to the code model
    // TODO: It would be nice to only tokenize the source files, not update the code model entirely.

    CppEditor::CppModelManager *modelManager = CppEditor::CppModelManager::instance();

    QSet<FilePath> filesToBeUpdated;
    const CppEditor::ProjectInfoList infoList = modelManager->projectInfos();
    for (const CppEditor::ProjectInfo::ConstPtr &info : infoList)
        filesToBeUpdated.unite(info->sourceFiles());

    modelManager->updateSourceFiles(filesToBeUpdated);
}

void CppTodoItemsScanner::documentUpdated(CPlusPlus::Document::Ptr doc)
{
    CppEditor::CppModelManager *modelManager = CppEditor::CppModelManager::instance();
    if (!modelManager->projectPart(doc->filePath()).isEmpty())
        processDocument(doc);
}

void CppTodoItemsScanner::processDocument(CPlusPlus::Document::Ptr doc)
{
    QList<TodoItem> itemList;
    CPlusPlus::TranslationUnit *translationUnit = doc->translationUnit();

    for (int i = 0; i < translationUnit->commentCount(); ++i) {

        // Get comment source
        CPlusPlus::Token token = doc->translationUnit()->commentAt(i);
        QByteArray source = doc->utf8Source().mid(token.bytesBegin(), token.bytes()).trimmed();

        if ((token.kind() == CPlusPlus::T_COMMENT) || (token.kind() == CPlusPlus::T_DOXY_COMMENT)) {
            // Remove trailing "*/"
            source = source.left(source.length() - 2);
        }

        // Process every line of the comment
        int lineNumber = 0;
        translationUnit->getPosition(token.utf16charsBegin(), &lineNumber);

        for (int from = 0, sz = source.size(); from < sz; ++lineNumber) {
            int to = source.indexOf('\n', from);
            if (to == -1)
                to = sz - 1;

            const char *start = source.constData() + from;
            const char *end = source.constData() + to;
            while (start != end && std::isspace((unsigned char)*start))
                ++start;
            while (start != end && std::isspace((unsigned char)*end))
                --end;
            const int length = end - start + 1;
            if (length > 0) {
                QString commentLine = QString::fromUtf8(start, length);
                processCommentLine(doc->filePath().toString(), commentLine, lineNumber, itemList);
            }

            from = to + 1;
        }
    }
    emit itemsFetched(doc->filePath().toString(), itemList);
}

}
}
