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

#include "qmlprofilercalltreeview.h"

#include <QtCore/QUrl>
#include <QtCore/QHash>

#include <QtGui/QHeaderView>
#include <QtGui/QStandardItemModel>

using namespace QmlProfiler::Internal;

struct BindingData
{
    BindingData() :
        displayname(0), filename(0), location(0), details(0),
        line(-1), rangeType(-1), level(-1), childrenHash(0), parentBinding(0) {}

    ~BindingData() {
        delete displayname;
        delete filename;
        delete location;
        delete childrenHash;
    }
    QString *displayname;
    QString *filename;
    QString *location;
    QString *details;
    int line;
    int rangeType;
    qint64 level;
    QHash<QString, BindingData *> *childrenHash;

    // reference to parent binding stored in the hash
    BindingData *parentBinding;
};

typedef QHash<QString, BindingData *> BindingHash;

enum ItemRole {
    LocationRole = Qt::UserRole+1,
    FilenameRole = Qt::UserRole+2,
    LineRole = Qt::UserRole+3
};

class QmlProfilerCallTreeView::QmlProfilerCallTreeViewPrivate
{
public:
    QmlProfilerCallTreeViewPrivate(QmlProfilerCallTreeView *qq) : q(qq) {}

    void recursiveClearHash(BindingHash *hash);
    void buildModelFromHash( BindingHash *hash, QStandardItem *parentItem );

    QmlProfilerCallTreeView *q;

    QStandardItemModel *m_model;
//  ToDo: avoid unnecessary allocations by using global hash
//    BindingHash m_globalHash;
    BindingHash m_rootHash;
    QList<BindingData *> m_bindingBuffer;
};

QmlProfilerCallTreeView::QmlProfilerCallTreeView(QWidget *parent) :
    QTreeView(parent), d(new QmlProfilerCallTreeViewPrivate(this))
{
    setObjectName("QmlProfilerCallTreeView");
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
}

QmlProfilerCallTreeView::~QmlProfilerCallTreeView()
{
    clean();
    delete d->m_model;
}

void QmlProfilerCallTreeView::clean()
{
    d->m_model->clear();
    d->m_model->setColumnCount(3);

    // clean the hashes
    d->recursiveClearHash(&d->m_rootHash);
//    d->recursiveClearHash(&d->m_globalHash);

    setHeaderLabels();
    setSortingEnabled(false);
}

void QmlProfilerCallTreeView::addRangedEvent(int type, int nestingLevel, int nestingInType, qint64 startTime, qint64 length,
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

    // ToDo: instead of allocating each new event, store them in the global hash
    BindingData *newBinding = new BindingData;
    newBinding->displayname = new QString(displayName);
    newBinding->filename = new QString(fileName);
    newBinding->line = line;
    newBinding->level = nestingLevel;
    newBinding->rangeType = type;
    newBinding->location = new QString(location);
    newBinding->details = new QString(details);
    newBinding->childrenHash = 0;

    d->m_bindingBuffer.prepend(newBinding);

    if (nestingLevel == 1) {
        // top level: insert buffered stuff
        BindingHash *currentHash = &(d->m_rootHash);
        BindingData *lastBinding = 0;
        int lastLevel = 0;

        while (d->m_bindingBuffer.length()) {
            BindingData *bindingInfo = d->m_bindingBuffer.at(0);
            // find the data's place
            if (bindingInfo->level > lastLevel) {
                currentHash = lastBinding ? lastBinding->childrenHash : &(d->m_rootHash);
                bindingInfo->parentBinding = lastBinding;
                ++lastLevel;
            } else if (bindingInfo->level == lastLevel) {
                bindingInfo->parentBinding = lastBinding->parentBinding;
            } else if (bindingInfo->level < lastLevel) {
                while (bindingInfo->level < lastLevel) {
                    bindingInfo->parentBinding = lastBinding->parentBinding ? lastBinding->parentBinding->parentBinding : 0;
                    currentHash = bindingInfo->parentBinding ? bindingInfo->parentBinding->childrenHash : &(d->m_rootHash);
                    --lastLevel;
                }
            }

            BindingHash::iterator it = currentHash->find(*bindingInfo->location);
            if (it == currentHash->end()) {
                bindingInfo->childrenHash = new BindingHash;
                currentHash->insert(*bindingInfo->location, bindingInfo);
                lastBinding = bindingInfo;
            } else {
                lastBinding = it.value();
                delete bindingInfo;
            }

            d->m_bindingBuffer.removeFirst();
        }
    }
}

void QmlProfilerCallTreeView::complete()
{
    // build the model from the hashed data
    d->buildModelFromHash( &d->m_rootHash, d->m_model->invisibleRootItem());

    expandAll();
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void QmlProfilerCallTreeView::jumpToItem(const QModelIndex &index)
{
    QStandardItem *clickedItem = d->m_model->itemFromIndex(index);
    QStandardItem *infoItem;
    if (clickedItem->parent())
        infoItem = clickedItem->parent()->child(clickedItem->row(), 0);
    else
        infoItem = d->m_model->item(index.row(), 0);

    int line = infoItem->data(LineRole).toInt();
    if (line == -1)
        return;
    QString fileName = infoItem->data(FilenameRole).toString();
    emit gotoSourceLocation(fileName, line);
}

void QmlProfilerCallTreeView::setHeaderLabels()
{
    d->m_model->setHeaderData(0, Qt::Horizontal, QVariant(tr("Location")));
    d->m_model->setHeaderData(1, Qt::Horizontal, QVariant(tr("Type")));
    d->m_model->setHeaderData(2, Qt::Horizontal, QVariant(tr("Details")));
}

void QmlProfilerCallTreeView::QmlProfilerCallTreeViewPrivate::recursiveClearHash(BindingHash *hash) {
    QHashIterator<QString, BindingData *> it(*hash);
    while (it.hasNext()) {
        it.next();
        if (it.value()->childrenHash)
            recursiveClearHash(it.value()->childrenHash);
        delete it.value();
    }
    hash->clear();
}

inline QString nameForType(int typeNumber)
{
    switch (typeNumber) {
    case 0: return QmlProfilerCallTreeView::tr("Paint");
    case 1: return QmlProfilerCallTreeView::tr("Compile");
    case 2: return QmlProfilerCallTreeView::tr("Create");
    case 3: return QmlProfilerCallTreeView::tr("Binding");
    case 4: return QmlProfilerCallTreeView::tr("Signal");
    }
    return QString();
}

void QmlProfilerCallTreeView::QmlProfilerCallTreeViewPrivate::buildModelFromHash( BindingHash *hash, QStandardItem *parentItem )
{
    QHashIterator<QString, BindingData *> it(*hash);

    while (it.hasNext()) {
        it.next();
        BindingData *binding = it.value();

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
        if (!binding->childrenHash->isEmpty())
            buildModelFromHash(binding->childrenHash, nameColumn);
    }
}
