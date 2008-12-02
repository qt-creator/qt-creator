/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
