/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilercallerview.h"

#include <QtCore/QUrl>
#include <QtCore/QHash>

#include <QtGui/QHeaderView>
#include <QtGui/QStandardItemModel>

using namespace QmlProfiler::Internal;

struct BindingData
{
    BindingData() : displayname(0) , filename(0) , location(0) , details(0),
    line(0), rangeType(0), level(-1), parentList(0) {}
    ~BindingData() {
        delete displayname;
        delete filename;
        delete location;
        delete parentList;
    }
    QString *displayname;
    QString *filename;
    QString *location;
    QString *details;
    int line;
    int rangeType;
    qint64 level;
    QList< BindingData *> *parentList;
};

typedef QHash<QString, BindingData *> BindingHash;
typedef QList<BindingData *> BindingList;

enum ItemRole {
    LocationRole = Qt::UserRole+1,
    FilenameRole = Qt::UserRole+2,
    LineRole = Qt::UserRole+3
};

class QmlProfilerCallerView::QmlProfilerCallerViewPrivate
{
public:
    QmlProfilerCallerViewPrivate(QmlProfilerCallerView *qq) : q(qq) {}

    void recursiveClearHash(BindingHash *hash);
    void buildModelFromList( BindingList *list, QStandardItem *parentItem, BindingList *listSoFar );

    QmlProfilerCallerView *q;

    QStandardItemModel *m_model;
    BindingHash m_rootHash;
    QHash<int, BindingList> m_pendingChildren;
    int m_lastLevel;
};

QmlProfilerCallerView::QmlProfilerCallerView(QWidget *parent) :
    QTreeView(parent), d(new QmlProfilerCallerViewPrivate(this))
{
    setObjectName("QmlProfilerCallerView");
    setRootIsDecorated(true);
    header()->setResizeMode(QHeaderView::Interactive);
    header()->setMinimumSectionSize(50);
    setSortingEnabled(false);
    setFrameStyle(QFrame::NoFrame);

    d->m_model = new QStandardItemModel(this);

    setModel(d->m_model);

    d->m_model->setColumnCount(3);
    setHeaderLabels();

    connect(this,SIGNAL(clicked(QModelIndex)), this,SLOT(jumpToItem(QModelIndex)));

    d->m_lastLevel = -1;
}

QmlProfilerCallerView::~QmlProfilerCallerView()
{
    clean();
    delete d->m_model;
}

void QmlProfilerCallerView::clean()
{
    d->m_model->clear();
    d->m_model->setColumnCount(3);

    foreach (int levelNumber, d->m_pendingChildren.keys())
        d->m_pendingChildren[levelNumber].clear();

    d->m_lastLevel = -1;

    // clean the hashes
    d->recursiveClearHash(&d->m_rootHash);

    setHeaderLabels();
    setSortingEnabled(false);
}

void QmlProfilerCallerView::addRangedEvent(int type, int nestingLevel, int nestingInType, qint64 startTime, qint64 length,
    const QStringList &data, const QString &fileName, int line)
{
    Q_UNUSED(startTime);
    Q_UNUSED(nestingInType);
    Q_UNUSED(length);

    const QChar colon = QLatin1Char(':');
    QString localName, displayName, location, details;

    if (fileName.isEmpty()) {
        displayName = tr("<bytecode>");
        location = QLatin1String("--");

    } else {
        localName = QUrl(fileName).toLocalFile();
        displayName = localName.mid(localName.lastIndexOf(QChar('/')) + 1) + colon + QString::number(line);
        location = fileName+colon+QString::number(line);
    }

    if (data.isEmpty())
        details = tr("Source code not available");
    else
        details = data.join(" ").replace('\n'," ");


    // New Data:  if it's not in the hash, put it there
    // if it's in the hash, get the reference from the hash
    BindingData *newBinding;
    BindingHash::iterator it = d->m_rootHash.find(location);
    if (it != d->m_rootHash.end()) {
        newBinding = it.value();
    } else {
        newBinding = new BindingData;
        newBinding->displayname = new QString(displayName);
        newBinding->filename = new QString(fileName);
        newBinding->line = line;
        newBinding->level = nestingLevel;
        newBinding->rangeType = type;
        newBinding->location = new QString(location);
        newBinding->details = new QString(details);
        newBinding->parentList = new BindingList();
        d->m_rootHash.insert(location, newBinding);
    }

    if (nestingLevel < d->m_lastLevel) {
        // I'm the parent of the former
        if (d->m_pendingChildren.contains(nestingLevel+1)) {
            foreach (BindingData *child, d->m_pendingChildren[nestingLevel + 1]) {
                if (!child->parentList->contains(newBinding))
                    child->parentList->append(newBinding);
            }
            d->m_pendingChildren[nestingLevel + 1].clear();
        }

    }

    if (nestingLevel > 1 && !d->m_pendingChildren[nestingLevel].contains(newBinding)) {
        // I'm not root... there will come a parent later
        d->m_pendingChildren[nestingLevel].append(newBinding);
    }

    d->m_lastLevel = nestingLevel;
}

void QmlProfilerCallerView::complete()
{
    // build the model from the hashed data
    BindingList bindingList = d->m_rootHash.values();
    BindingList emptyList;
    d->buildModelFromList(&bindingList, d->m_model->invisibleRootItem(), &emptyList);

    expandAll();
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    collapseAll();
}

void QmlProfilerCallerView::jumpToItem(const QModelIndex &index)
{
    QStandardItem *clickedItem = d->m_model->itemFromIndex(index);
    QStandardItem *infoItem;
    if (clickedItem->parent())
        infoItem = clickedItem->parent()->child(clickedItem->row(), 0);
    else
        infoItem = d->m_model->item(index.row(), 0);

    int line = infoItem->data(Qt::UserRole+3).toInt();
    if (line == -1)
        return;
    QString fileName = infoItem->data(Qt::UserRole+2).toString();
    emit gotoSourceLocation(fileName, line);
}

void QmlProfilerCallerView::setHeaderLabels()
{
    d->m_model->setHeaderData(0, Qt::Horizontal, QVariant(tr("Location")));
    d->m_model->setHeaderData(1, Qt::Horizontal, QVariant(tr("Type")));
    d->m_model->setHeaderData(2, Qt::Horizontal, QVariant(tr("Details")));
}

// ToDo: property clean the stored data
void QmlProfilerCallerView::QmlProfilerCallerViewPrivate::recursiveClearHash(BindingHash *hash) {
    QHashIterator<QString, BindingData *> it(*hash);
    while (it.hasNext()) {
        it.next();
        delete it.value();
    }
    hash->clear();
}

inline QString nameForType(int typeNumber)
{
    switch (typeNumber) {
    case 0: return QmlProfilerCallerView::tr("Paint");
    case 1: return QmlProfilerCallerView::tr("Compile");
    case 2: return QmlProfilerCallerView::tr("Create");
    case 3: return QmlProfilerCallerView::tr("Binding");
    case 4: return QmlProfilerCallerView::tr("Signal");
    }
    return QString();
}

void QmlProfilerCallerView::QmlProfilerCallerViewPrivate::buildModelFromList( BindingList *list, QStandardItem *parentItem, BindingList *listSoFar )
{
    foreach (BindingData *binding, *list) {
        if (listSoFar->contains(binding))
            continue;

        QStandardItem *nameColumn = new QStandardItem(*binding->displayname);
        nameColumn->setEditable(false);

        QStandardItem *typeColumn = new QStandardItem(nameForType(binding->rangeType));
        typeColumn->setEditable(false);

        QStandardItem *detailsColumn = new QStandardItem(*binding->details);
        detailsColumn->setEditable(false);

        QStandardItem *firstColumn = nameColumn;
        firstColumn->setData(QVariant(*binding->location),LocationRole);
        firstColumn->setData(QVariant(*binding->filename),FilenameRole);
        firstColumn->setData(QVariant(binding->line),LineRole);

        QList<QStandardItem *> newRow;
        newRow << nameColumn << typeColumn << detailsColumn;
        parentItem->appendRow(newRow);
        if (!binding->parentList->isEmpty()) {
            // avoid infinite loops due to recursive functions
            BindingList newParentList(*listSoFar);
            newParentList.append(binding);

            buildModelFromList(binding->parentList, nameColumn, &newParentList);
        }
    }
}
