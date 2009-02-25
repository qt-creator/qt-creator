/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SEARCHRESULTTREEVIEW_H
#define SEARCHRESULTTREEVIEW_H

#include <QtGui/QTreeView>
#include <QtGui/QKeyEvent>

namespace Find {
namespace Internal {

class SearchResultTreeModel;

class SearchResultTreeView : public QTreeView
{
    Q_OBJECT

public:
    SearchResultTreeView(QWidget *parent = 0);
    void setAutoExpandResults(bool expand);

signals:
    void jumpToSearchResult(int index, const QString &fileName, int lineNumber,
        int searchTermStart, int searchTermLength);

public slots:
    void clear();
    void appendResultLine(int index, const QString &fileName, int lineNumber, const QString &lineText,
        int searchTermStart, int searchTermLength);

private slots:
    void emitJumpToSearchResult(const QModelIndex &index);

protected:
    void keyPressEvent(QKeyEvent *e);

    SearchResultTreeModel *m_model;
    bool m_autoExpandResults;
};

} // namespace Internal
} // namespace Find

#endif // SEARCHRESULTTREEVIEW_H
