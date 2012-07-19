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

#ifndef XBELSUPPORT_H
#define XBELSUPPORT_H

#include <QIcon>
#include <QXmlStreamReader>

QT_BEGIN_NAMESPACE
class QIODevice;
class QStandardItem;
QT_END_NAMESPACE

class BookmarkModel;

namespace Help {
namespace Internal {

class XbelWriter : public QXmlStreamWriter
{
public:
    XbelWriter(BookmarkModel *model);
    void writeToFile(QIODevice *device);

private:
    void writeData(QStandardItem *item);

private:
    BookmarkModel *treeModel;
};

class XbelReader : public QXmlStreamReader
{
public:
    XbelReader(BookmarkModel *tree, BookmarkModel *list);
    bool readFromFile(QIODevice *device);

private:
    void readXBEL();
    void readUnknownElement();
    void readFolder(QStandardItem *item);
    void readBookmark(QStandardItem *item);
    QStandardItem* createChildItem(QStandardItem *item);

private:
    QIcon folderIcon;
    QIcon bookmarkIcon;

    BookmarkModel *treeModel;
    BookmarkModel *listModel;
};

    }   // Internal
}   // Help

#endif  // XBELSUPPORT_H
