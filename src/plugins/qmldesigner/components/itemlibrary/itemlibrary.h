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

#ifndef ITEMLIBRARY_H
#define ITEMLIBRARY_H

#include <QtGui/QFrame>

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QPixmap;
class QMimeData;
QT_END_NAMESPACE

namespace QmlDesigner {

class ItemLibraryPrivate;
class MetaInfo;
class ItemLibraryInfo;

class ItemLibrary : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY(ItemLibrary)

public:
    ItemLibrary(QWidget *parent = 0);
    virtual ~ItemLibrary();

    void addItemLibraryInfo(const ItemLibraryInfo &ItemLibraryInfo);
    void setMetaInfo(const MetaInfo &metaInfo);

public Q_SLOTS:
    void itemLibraryButtonToggled();
    void resourcesButtonToggled();

    void setSearchFilter(const QString &nameFilter);
    void setResourcePath(const QString &resourcePath);

    void startDragAndDrop(int itemLibId);
    void showItemInfo(int itemLibId);

signals:
    void itemActivated(const QString& itemName);
    void expandAllItems();

private:
    ItemLibraryPrivate *m_d;
};

}

#endif // ITEMLIBRARY_H

