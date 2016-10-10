/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "xbelsupport.h"

#include <bookmarkmanager.h>

#include <utils/utilsicons.h>

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
    writeDTD("<!DOCTYPE xbel>");
    writeStartElement("xbel");
    writeAttribute("version", "1.0");

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

    if (entry.url == "Folder") {
        writeStartElement("folder");

        entry.folded = !child->data(Qt::UserRole + 11).toBool();
        writeAttribute("folded",
            entry.folded ? QLatin1String("yes") : QLatin1String("no"));

        writeTextElement("title", entry.title);

        for (int i = 0; i < child->rowCount(); ++i)
            writeData(child->child(i));

        writeEndElement();
    } else {
        writeStartElement("bookmark");
        writeAttribute("href", entry.url);
        writeTextElement("title", entry.title);
        writeEndElement();
    }
}


// #pragma mark -- XbelReader


XbelReader::XbelReader(BookmarkModel *tree, BookmarkModel *list)
    : QXmlStreamReader()
    , treeModel(tree)
    , listModel(list)
{
    bookmarkIcon = Utils::Icons::BOOKMARK.icon();
    folderIcon = QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon);
}

bool XbelReader::readFromFile(QIODevice *device)
{
    setDevice(device);

    while (!atEnd()) {
        readNext();

        if (isStartElement()) {
            if (name() == "xbel"
                && attributes().value("version")
                    == "1.0") {
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
            if (name() == "folder")
                readFolder(0);
            else if (name() == "bookmark")
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
    folder->setData("Folder", Qt::UserRole + 10);

    bool expanded =
        (attributes().value("folded") != "no");
    folder->setData(expanded, Qt::UserRole + 11);

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement()) {
            if (name() == "title")
                folder->setText(readElementText());
            else if (name() == "folder")
                readFolder(folder);
            else if (name() == "bookmark")
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
    bookmark->setData(attributes().value("href").toString(),
        Qt::UserRole + 10);

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement()) {
            if (name() == "title")
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
