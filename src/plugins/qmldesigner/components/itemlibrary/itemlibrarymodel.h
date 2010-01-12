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

#ifndef ITEMLIBRARYMODEL_H
#define ITEMLIBRARYMODEL_H

#include <QtGui/QStandardItemModel>
#include <QtGui/QTreeView>
#include <QDrag>
#include <QDebug>
#include <QTimeLine>

class QLabel;

namespace QmlDesigner {

class ItemLibraryInfo;

namespace Internal {

// QStandardItemModel-based model for the widget box.
class ItemLibraryModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit ItemLibraryModel(QObject *parent = 0);
    void addItemLibraryInfo(const ItemLibraryInfo &ItemLibraryInfo);


    QStandardItem *findCategoryItem(const QString &category);

    virtual Qt::DropActions supportedDragActions() const;
    virtual Qt::DropActions supportedDropActions() const;

    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
};

// ItemLibraryTreeView with Drag implementation
class ItemLibraryTreeView : public QTreeView {
    Q_OBJECT
    public:
    explicit ItemLibraryTreeView(QWidget *parent = 0);

    virtual void startDrag(Qt::DropActions supportedActions);

    void setRealModel(QStandardItemModel *model) { m_model = model; }

signals:
        void itemActivated(const QString &itemName);
private slots:
        void activateItem( const QModelIndex & index);

private:
    QPixmap m_smallImage, m_bigImage;
    QStandardItemModel *m_model;
};



} // namespace Internal
} // namespace QmlDesigner
#endif // ITEMLIBRARYMODEL_H
