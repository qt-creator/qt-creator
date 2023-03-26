// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todoitemsscanner.h"
#include "lineparser.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>

namespace Todo {
namespace Internal {

TodoItemsScanner::TodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    QObject(parent), m_keywordList(keywordList)
{
}

void TodoItemsScanner::setParams(const KeywordList &keywordList)
{
    m_keywordList = keywordList;
    scannerParamsChanged();
}


// Descendants can use this to process comment lines
void TodoItemsScanner::processCommentLine(const QString &fileName, const QString &comment,
                                          unsigned lineNumber, QList<TodoItem> &outItemList)
{
    LineParser parser(m_keywordList);
    QList<TodoItem> newItemList = parser.parse(comment);

    for (int i = 0; i < newItemList.count(); ++i) {
        newItemList[i].line = lineNumber;
        newItemList[i].file = Utils::FilePath::fromString(fileName);
    }

    outItemList << newItemList;
}

}
}
