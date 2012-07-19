/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SEARCHRESULTTREEVIEW_H
#define SEARCHRESULTTREEVIEW_H

#include "searchresultwindow.h"

#include <QTreeView>

namespace Find {
namespace Internal {

class SearchResultTreeModel;

class SearchResultTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit SearchResultTreeView(QWidget *parent = 0);

    void setAutoExpandResults(bool expand);
    void setTextEditorFont(const QFont &font);

    SearchResultTreeModel *model() const;
    void addResults(const QList<Find::SearchResultItem> &items, SearchResult::AddMode mode);

signals:
    void jumpToSearchResult(const SearchResultItem &item);

public slots:
    void clear();
    void emitJumpToSearchResult(const QModelIndex &index);

protected:
    void keyPressEvent(QKeyEvent *e);

    SearchResultTreeModel *m_model;
    bool m_autoExpandResults;
};

} // namespace Internal
} // namespace Find

#endif // SEARCHRESULTTREEVIEW_H
