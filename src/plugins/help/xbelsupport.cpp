// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xbelsupport.h"

#include "helptr.h"

#include <bookmarkmanager.h>

#include <utils/utilsicons.h>

#include <QApplication>

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
            if (name() == QLatin1String("xbel")
                && attributes().value("version") == QLatin1String("1.0")) {
                readXBEL();
            } else {
                raiseError(Tr::tr("The file is not an XBEL version 1.0 file."));
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
                readFolder(nullptr);
            else if (name() == QLatin1String("bookmark"))
                readBookmark(nullptr);
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

    const bool expanded = attributes().value("folded") != QLatin1String("no");
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
    bookmark->setText(Tr::tr("Unknown title"));
    bookmark->setData(attributes().value("href").toString(),
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
    auto childItem = new QStandardItem;
    childItem->setEditable(false);

    if (item)
        item->appendRow(childItem);
    else
        treeModel->appendRow(childItem);

    return childItem;
}
