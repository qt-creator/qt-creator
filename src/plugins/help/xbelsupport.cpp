/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "xbelsupport.h"
#include "bookmarkmanager.h"

#include <QCoreApplication>

using namespace Help::Internal;

struct Bookmark {
    QString title;
    QString url;
    bool folded;
};

XbelWriter::XbelWriter(BookmarkModel *model)
    : QXmlStreamWriter()
    , treeModel(model)
{
    setAutoFormatting(true);
}

void XbelWriter::writeToFile(QIODevice *device)
{
    setDevice(device);

    writeStartDocument();
    writeDTD(QLatin1String("<!DOCTYPE xbel>"));
    writeStartElement(QLatin1String("xbel"));
    writeAttribute(QLatin1String("version"), QLatin1String("1.0"));

    QStandardItem *root = treeModel->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); ++i)
        writeData(root->child(i));

    writeEndDocument();
}

void XbelWriter::writeData(QStandardItem *child)
{
    Bookmark entry;
    entry.title = child->data(Qt::DisplayRole).toString();
    entry.url = child->data(Qt::UserRole + 10).toString();

    if (entry.url == QLatin1String("Folder")) {
        writeStartElement(QLatin1String("folder"));

        entry.folded = !child->data(Qt::UserRole + 11).toBool();
        writeAttribute(QLatin1String("folded"),
            entry.folded ? QLatin1String("yes") : QLatin1String("no"));

        writeTextElement(QLatin1String("title"), entry.title);

        for (int i = 0; i < child->rowCount(); ++i)
            writeData(child->child(i));

        writeEndElement();
    } else {
        writeStartElement(QLatin1String("bookmark"));
        writeAttribute(QLatin1String("href"), entry.url);
        writeTextElement(QLatin1String("title"), entry.title);
        writeEndElement();
    }
}


// #pragma mark -- XbelReader


XbelReader::XbelReader(BookmarkModel *tree, BookmarkModel *list)
    : QXmlStreamReader()
    , treeModel(tree)
    , listModel(list)
{
    bookmarkIcon = QIcon(QLatin1String(":/help/images/bookmark.png"));
    folderIcon = QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon);
}

bool XbelReader::readFromFile(QIODevice *device)
{
    setDevice(device);

    while (!atEnd()) {
        readNext();

        if (isStartElement()) {
            if (name() == QLatin1String("xbel")
                && attributes().value(QLatin1String("version"))
                    == QLatin1String("1.0")) {
                readXBEL();
            } else {
                raiseError(QCoreApplication::translate("Help::Internal::XbelReader", "The file is not an XBEL version 1.0 file."));
            }
        }
    }

    return !error();
}

void XbelReader::readXBEL()
{
    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement()) {
            if (name() == QLatin1String("folder"))
                readFolder(0);
            else if (name() == QLatin1String("bookmark"))
                readBookmark(0);
            else
                readUnknownElement();
        }
    }
}

void XbelReader::readUnknownElement()
{
    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            readUnknownElement();
    }
}

void XbelReader::readFolder(QStandardItem *item)
{
    QStandardItem *folder = createChildItem(item);
    folder->setIcon(folderIcon);
    folder->setData(QLatin1String("Folder"), Qt::UserRole + 10);

    bool expanded =
        (attributes().value(QLatin1String("folded")) != QLatin1String("no"));
    folder->setData(expanded, Qt::UserRole + 11);

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement()) {
            if (name() == QLatin1String("title"))
                folder->setText(readElementText());
            else if (name() == QLatin1String("folder"))
                readFolder(folder);
            else if (name() == QLatin1String("bookmark"))
                readBookmark(folder);
            else
                readUnknownElement();
        }
    }
}

void XbelReader::readBookmark(QStandardItem *item)
{
    QStandardItem *bookmark = createChildItem(item);
    bookmark->setIcon(bookmarkIcon);
    bookmark->setText(QCoreApplication::translate("Help::Internal::XbelReader", "Unknown title"));
    bookmark->setData(attributes().value(QLatin1String("href")).toString(),
        Qt::UserRole + 10);

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement()) {
            if (name() == QLatin1String("title"))
                bookmark->setText(readElementText());
            else
                readUnknownElement();
        }
    }

    listModel->appendRow(bookmark->clone());
}

QStandardItem *XbelReader::createChildItem(QStandardItem *item)
{
    QStandardItem *childItem = new QStandardItem();
    childItem->setEditable(false);

    if (item)
        item->appendRow(childItem);
    else
        treeModel->appendRow(childItem);

    return childItem;
}
