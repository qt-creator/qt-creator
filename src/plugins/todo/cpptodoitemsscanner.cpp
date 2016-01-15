/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
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

#include "cpptodoitemsscanner.h"

#include <cpptools/projectinfo.h>

#include <projectexplorer/project.h>

#include <cplusplus/TranslationUnit.h>

#include <cctype>

namespace Todo {
namespace Internal {

CppTodoItemsScanner::CppTodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    TodoItemsScanner(keywordList, parent)
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    connect(modelManager, &CppTools::CppModelManager::documentUpdated,
            this, &CppTodoItemsScanner::documentUpdated, Qt::DirectConnection);

    setParams(keywordList);
}

void CppTodoItemsScanner::scannerParamsChanged()
{
    // We need to rescan everything known to the code model
    // TODO: It would be nice to only tokenize the source files, not update the code model entirely.

    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    QSet<QString> filesToBeUpdated;
    foreach (const CppTools::ProjectInfo &info, modelManager->projectInfos())
        filesToBeUpdated.unite(info.project().data()->files(ProjectExplorer::Project::SourceFiles).toSet());

    modelManager->updateSourceFiles(filesToBeUpdated);
}

void CppTodoItemsScanner::documentUpdated(CPlusPlus::Document::Ptr doc)
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    if (!modelManager->projectPart(doc->fileName()).isEmpty())
        processDocument(doc);
}

void CppTodoItemsScanner::processDocument(CPlusPlus::Document::Ptr doc)
{
    QList<TodoItem> itemList;
    CPlusPlus::TranslationUnit *translationUnit = doc->translationUnit();

    for (unsigned i = 0; i < translationUnit->commentCount(); ++i) {

        // Get comment source
        CPlusPlus::Token token = doc->translationUnit()->commentAt(i);
        QByteArray source = doc->utf8Source().mid(token.bytesBegin(), token.bytes()).trimmed();

        if ((token.kind() == CPlusPlus::T_COMMENT) || (token.kind() == CPlusPlus::T_DOXY_COMMENT)) {
            // Remove trailing "*/"
            source = source.left(source.length() - 2);
        }

        // Process every line of the comment
        unsigned lineNumber = 0;
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
                processCommentLine(doc->fileName(), commentLine, lineNumber, itemList);
            }

            from = to + 1;
        }
    }
    emit itemsFetched(doc->fileName(), itemList);
}

}
}
