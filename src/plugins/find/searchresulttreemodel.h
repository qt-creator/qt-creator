/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SEARCHRESULTTREEMODEL_H
#define SEARCHRESULTTREEMODEL_H

#include <QtCore/QAbstractItemModel>
#include <QtGui/QFont>

namespace Find {
namespace Internal {

class SearchResultTreeItem;
class SearchResultTextRow;
class SearchResultFile;

class SearchResultTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SearchResultTreeModel(QObject *parent = 0);
    ~SearchResultTreeModel();

    void setShowReplaceUI(bool show);
    void setTextEditorFont(const QFont &font);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QModelIndex next(const QModelIndex &idx) const;
    QModelIndex prev(const QModelIndex &idx) const;

signals:
    void jumpToSearchResult(const QString &fileName, int lineNumber,
                            int searchTermStart, int searchTermLength);

public slots:
    void clear();
    void appendResultLine(int index, int lineNumber, const QString &rowText,
                          int searchTermStart, int searchTermLength);
    void appendResultLine(int index, const QString &fileName, int lineNumber, const QString &rowText,
                          int searchTermStart, int searchTermLength);

private:
    void appendResultFile(const QString &fileName);
    QVariant data(const SearchResultTextRow *row, int role) const;
    QVariant data(const SearchResultFile *file, int role) const;
    void initializeData();
    void disposeData();

    SearchResultTreeItem *m_rootItem;
    SearchResultFile *m_lastAppendedResultFile;
    QFont m_textEditorFont;
    bool m_showReplaceUI;
};

} // namespace Internal
} // namespace Find

#endif // SEARCHRESULTTREEMODEL_H
