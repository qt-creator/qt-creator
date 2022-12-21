// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
