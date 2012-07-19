/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "todoitemsscanner.h"
#include "lineparser.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>

namespace Todo {
namespace Internal {

TodoItemsScanner::TodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    QObject(parent)
{
    setKeywordList(keywordList);
}

void TodoItemsScanner::setKeywordList(const KeywordList &keywordList)
{
    m_keywordList = keywordList;
    keywordListChanged();
}

// Descendants can override and make a request for full rescan here if needed
void TodoItemsScanner::keywordListChanged()
{
}

// Descendants can use this to process comment lines
void TodoItemsScanner::processCommentLine(const QString &fileName, const QString &comment,
    unsigned lineNumber, QList<TodoItem> &outItemList)
{
    LineParser parser(m_keywordList);
    QList<TodoItem> newItemList = parser.parse(comment);

    for (int i = 0; i < newItemList.count(); ++i) {
        newItemList[i].line = lineNumber;
        newItemList[i].file = fileName;
    }

    outItemList << newItemList;
}

}
}
