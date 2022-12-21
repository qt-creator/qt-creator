// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "todoitem.h"
#include "keyword.h"

#include <QObject>

namespace Todo {
namespace Internal {

// TodoItemsScanner is an abstract class

class TodoItemsScanner : public QObject
{
    Q_OBJECT

public:
    explicit TodoItemsScanner(const KeywordList &keywordList, QObject *parent = nullptr);
    void setParams(const KeywordList &keywordList);

signals:
    void itemsFetched(const QString &fileName, const QList<TodoItem> &items);

protected:
    KeywordList m_keywordList;

    // Descendants can override and make a request for full rescan here if needed
    virtual void scannerParamsChanged() = 0;

    void processCommentLine(const QString &fileName, const QString &comment,
                            unsigned lineNumber, QList<TodoItem> &outItemList);
};

}
}
