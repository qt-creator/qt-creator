/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qv8profilereventview.h"

#include <QUrl>
#include <QHash>

#include <QStandardItem>
#include <QHeaderView>

#include <QApplication>
#include <QClipboard>

#include <QContextMenuEvent>
#include <QDebug>

#include <coreplugin/minisplitter.h>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "qmlprofilerviewmanager.h"
#include "qmlprofilertool.h"
#include "qv8profilerdatamodel.h"
#include <QMenu>

#include <utils/qtcassert.h>

using namespace QmlDebug;

namespace QmlProfiler {
namespace Internal {

enum ItemRole {
    SortRole = Qt::UserRole + 1, // Sort by data, not by displayed text
    FilenameRole,
    LineRole,
    TypeIdRole
};

////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////

class V8ViewItem : public QStandardItem
{
public:
    V8ViewItem(const QString &text) : QStandardItem(text) {}

    virtual bool operator<(const QStandardItem &other) const
    {
        // first column is special
        if (column() == 0) {
            int filenameDiff = QUrl(data(FilenameRole).toString()).fileName().compare(
                    QUrl(other.data(FilenameRole).toString()).fileName(), Qt::CaseInsensitive);
            return filenameDiff != 0 ? filenameDiff < 0 :
                                       data(LineRole).toInt() < other.data(LineRole).toInt();
        } else if (data(SortRole).type() == QVariant::String) {
            // Strings should be case-insensitive compared
            return data(SortRole).toString().compare(other.data(SortRole).toString(),
                                                         Qt::CaseInsensitive) < 0;
        } else {
            // For everything else the standard comparison should be OK
            return QStandardItem::operator<(other);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////

class QV8ProfilerEventsWidget::QV8ProfilerEventsWidgetPrivate
{
public:
    QV8ProfilerEventsWidgetPrivate(QV8ProfilerEventsWidget *qq):q(qq) {}
    ~QV8ProfilerEventsWidgetPrivate() {}

    QV8ProfilerEventsWidget *q;

    Analyzer::IAnalyzerTool *m_profilerTool;
    QmlProfilerViewManager *m_viewContainer;

    QV8ProfilerEventsMainView *m_eventTree;
    QV8ProfilerEventRelativesView *m_eventChildren;
    QV8ProfilerEventRelativesView *m_eventParents;

    QV8ProfilerDataModel *v8Model;
};

QV8ProfilerEventsWidget::QV8ProfilerEventsWidget(QWidget *parent,
                                 Analyzer::IAnalyzerTool *profilerTool,
                                 QmlProfilerViewManager *container,
                                 QmlProfilerModelManager *profilerModelManager )
    : QWidget(parent), d(new QV8ProfilerEventsWidgetPrivate(this))
{
    setObjectName(QLatin1String("QmlProfilerV8ProfileView"));

    d->v8Model = profilerModelManager->v8Model();

    d->m_eventTree = new QV8ProfilerEventsMainView(this, d->v8Model);
    connect(d->m_eventTree, SIGNAL(gotoSourceLocation(QString,int,int)), this, SIGNAL(gotoSourceLocation(QString,int,int)));

    d->m_eventChildren = new QV8ProfilerEventRelativesView(d->v8Model,
                                                           QV8ProfilerEventRelativesView::ChildrenView,
                                                           this);
    d->m_eventParents = new QV8ProfilerEventRelativesView(d->v8Model,
                                                          QV8ProfilerEventRelativesView::ParentsView,
                                                          this);
    connect(d->m_eventTree, SIGNAL(typeSelected(int)), d->m_eventChildren, SLOT(displayType(int)));
    connect(d->m_eventTree, SIGNAL(typeSelected(int)), d->m_eventParents, SLOT(displayType(int)));
    connect(d->m_eventChildren, SIGNAL(typeClicked(int)), d->m_eventTree, SLOT(selectType(int)));
    connect(d->m_eventParents, SIGNAL(typeClicked(int)), d->m_eventTree, SLOT(selectType(int)));
    connect(d->v8Model, SIGNAL(changed()), this, SLOT(updateEnabledState()));

    // widget arrangement
    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0,0,0,0);
    groupLayout->setSpacing(0);

    Core::MiniSplitter *splitterVertical = new Core::MiniSplitter;
    splitterVertical->addWidget(d->m_eventTree);
    Core::MiniSplitter *splitterHorizontal = new Core::MiniSplitter;
    splitterHorizontal->addWidget(d->m_eventParents);
    splitterHorizontal->addWidget(d->m_eventChildren);
    splitterHorizontal->setOrientation(Qt::Horizontal);
    splitterVertical->addWidget(splitterHorizontal);
    splitterVertical->setOrientation(Qt::Vertical);
    splitterVertical->setStretchFactor(0,5);
    splitterVertical->setStretchFactor(1,2);
    groupLayout->addWidget(splitterVertical);
    setLayout(groupLayout);

    d->m_profilerTool = profilerTool;
    d->m_viewContainer = container;
    setEnabled(false);
}

QV8ProfilerEventsWidget::~QV8ProfilerEventsWidget()
{
    delete d;
}

void QV8ProfilerEventsWidget::updateEnabledState()
{
    setEnabled(!d->v8Model->isEmpty());
}

void QV8ProfilerEventsWidget::clear()
{
    d->m_eventTree->clear();
    d->m_eventChildren->clear();
    d->m_eventParents->clear();
    setEnabled(false);
}

QModelIndex QV8ProfilerEventsWidget::selectedItem() const
{
    return d->m_eventTree->selectedItem();
}

void QV8ProfilerEventsWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    QTC_ASSERT(d->m_viewContainer, return;);

    QMenu menu;
    QAction *copyRowAction = 0;
    QAction *copyTableAction = 0;

    QmlProfilerTool *profilerTool = qobject_cast<QmlProfilerTool *>(d->m_profilerTool);
    QPoint position = ev->globalPos();

    if (profilerTool) {
        QList <QAction *> commonActions = profilerTool->profilerContextMenuActions();
        foreach (QAction *act, commonActions) {
            menu.addAction(act);
        }
    }

    if (mouseOnTable(position)) {
        menu.addSeparator();
        if (selectedItem().isValid())
            copyRowAction = menu.addAction(QCoreApplication::translate("QmlProfiler::Internal::QmlProfilerEventsWidget", "Copy Row"));
        copyTableAction = menu.addAction(QCoreApplication::translate("QmlProfiler::Internal::QmlProfilerEventsWidget", "Copy Table"));
    }

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == copyRowAction)
            copyRowToClipboard();
        if (selectedAction == copyTableAction)
            copyTableToClipboard();
    }
}

void QV8ProfilerEventsWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    emit resized();
}

bool QV8ProfilerEventsWidget::mouseOnTable(const QPoint &position) const
{
    QPoint tableTopLeft = d->m_eventTree->mapToGlobal(QPoint(0,0));
    QPoint tableBottomRight = d->m_eventTree->mapToGlobal(QPoint(d->m_eventTree->width(), d->m_eventTree->height()));
    return (position.x() >= tableTopLeft.x() && position.x() <= tableBottomRight.x() && position.y() >= tableTopLeft.y() && position.y() <= tableBottomRight.y());
}

void QV8ProfilerEventsWidget::copyTableToClipboard() const
{
    d->m_eventTree->copyTableToClipboard();
}

void QV8ProfilerEventsWidget::copyRowToClipboard() const
{
    d->m_eventTree->copyRowToClipboard();
}

void QV8ProfilerEventsWidget::updateSelectedType(int typeId) const
{
    if (d->m_eventTree->selectedTypeId() != typeId)
        d->m_eventTree->selectType(typeId);
}

void QV8ProfilerEventsWidget::selectBySourceLocation(const QString &filename, int line, int column)
{
    // This slot is used to connect the javascript pane with the qml events pane
    // Our javascript trace data does not store column information
    // thus we ignore it here
    Q_UNUSED(column);
    d->m_eventTree->selectEventByLocation(filename, line);
}

////////////////////////////////////////////////////////////////////////////////////

class QV8ProfilerEventsMainView::QV8ProfilerEventsMainViewPrivate
{
public:
    QV8ProfilerEventsMainViewPrivate(QV8ProfilerEventsMainView *qq) : q(qq) {}

    void buildV8ModelFromList( const QList<QV8ProfilerDataModel::QV8EventData *> &list );
    int getFieldCount();

    QString textForItem(QStandardItem *item, bool recursive) const;


    QV8ProfilerEventsMainView *q;

    QV8ProfilerDataModel *m_v8Model;
    QStandardItemModel *m_model;
    QList<bool> m_fieldShown;
    QHash<int, int> m_columnIndex; // maps field enum to column index
    int m_firstNumericColumn;
    bool m_preventSelectBounce;
};


////////////////////////////////////////////////////////////////////////////////////

QV8ProfilerEventsMainView::QV8ProfilerEventsMainView(QWidget *parent,
                                   QV8ProfilerDataModel *v8Model)
: QmlProfilerTreeView(parent), d(new QV8ProfilerEventsMainViewPrivate(this))
{
    setObjectName(QLatin1String("QmlProfilerEventsTable"));
    setSortingEnabled(false);

    d->m_model = new QStandardItemModel(this);
    d->m_model->setSortRole(SortRole);
    setModel(d->m_model);
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(jumpToItem(QModelIndex)));

    d->m_v8Model = v8Model;
    connect(d->m_v8Model, SIGNAL(changed()), this, SLOT(buildModel()));
    d->m_firstNumericColumn = 0;
    d->m_preventSelectBounce = false;

    setFieldViewable(Name, true);
    setFieldViewable(Type, false);
    setFieldViewable(TimeInPercent, true);
    setFieldViewable(TotalTime, true);
    setFieldViewable(SelfTimeInPercent, true);
    setFieldViewable(SelfTime, true);
    setFieldViewable(CallCount, false);
    setFieldViewable(TimePerCall, false);
    setFieldViewable(MaxTime, false);
    setFieldViewable(MinTime, false);
    setFieldViewable(MedianTime, false);
    setFieldViewable(Details, true);

    buildModel();
}

QV8ProfilerEventsMainView::~QV8ProfilerEventsMainView()
{
    clear();
    delete d->m_model;
    delete d;
}

void QV8ProfilerEventsMainView::setFieldViewable(Fields field, bool show)
{
    if (field < MaxFields) {
        int length = d->m_fieldShown.count();
        if (field >= length) {
            for (int i=length; i<MaxFields; i++)
                d->m_fieldShown << false;
        }
        d->m_fieldShown[field] = show;
    }
}


void QV8ProfilerEventsMainView::setHeaderLabels()
{
    int fieldIndex = 0;
    d->m_firstNumericColumn = 0;

    d->m_columnIndex.clear();
    if (d->m_fieldShown[Name]) {
        d->m_columnIndex[Name] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(Location)));
        d->m_firstNumericColumn++;
    }
    if (d->m_fieldShown[Type]) {
        d->m_columnIndex[Type] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(Type)));
        d->m_firstNumericColumn++;
    }
    if (d->m_fieldShown[TimeInPercent]) {
        d->m_columnIndex[TimeInPercent] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(TimeInPercent)));
    }
    if (d->m_fieldShown[TotalTime]) {
        d->m_columnIndex[TotalTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(TotalTime)));
    }
    if (d->m_fieldShown[SelfTimeInPercent]) {
        d->m_columnIndex[Type] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(SelfTimeInPercent)));
    }
    if (d->m_fieldShown[SelfTime]) {
        d->m_columnIndex[SelfTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(SelfTime)));
    }
    if (d->m_fieldShown[CallCount]) {
        d->m_columnIndex[CallCount] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(CallCount)));
    }
    if (d->m_fieldShown[TimePerCall]) {
        d->m_columnIndex[TimePerCall] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(TimePerCall)));
    }
    if (d->m_fieldShown[MedianTime]) {
        d->m_columnIndex[MedianTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(MedianTime)));
    }
    if (d->m_fieldShown[MaxTime]) {
        d->m_columnIndex[MaxTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(MaxTime)));
    }
    if (d->m_fieldShown[MinTime]) {
        d->m_columnIndex[MinTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(MinTime)));
    }
    if (d->m_fieldShown[Details]) {
        d->m_columnIndex[Details] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(displayHeader(Details)));
    }
}

void QV8ProfilerEventsMainView::clear()
{
    d->m_model->clear();
    d->m_model->setColumnCount(d->getFieldCount());

    setHeaderLabels();
    setSortingEnabled(false);
}

int QV8ProfilerEventsMainView::QV8ProfilerEventsMainViewPrivate::getFieldCount()
{
    int count = 0;
    for (int i=0; i < m_fieldShown.count(); ++i)
        if (m_fieldShown[i])
            count++;
    return count;
}

void QV8ProfilerEventsMainView::buildModel()
{
    clear();
    d->buildV8ModelFromList( d->m_v8Model->getV8Events() );

    setRootIsDecorated(false);
    setSortingEnabled(true);
    sortByColumn(d->m_firstNumericColumn,Qt::DescendingOrder);

    expandAll();
    if (d->m_fieldShown[Name])
        resizeColumnToContents(0);

    if (d->m_fieldShown[Type])
        resizeColumnToContents(d->m_fieldShown[Name]?1:0);
    collapseAll();
}

void QV8ProfilerEventsMainView::QV8ProfilerEventsMainViewPrivate::buildV8ModelFromList(
        const QList<QV8ProfilerDataModel::QV8EventData *> &list)
{
    for (int index = 0; index < list.count(); index++) {
        QV8ProfilerDataModel::QV8EventData *v8event = list.at(index);
        QList<QStandardItem *> newRow;

        if (m_fieldShown[Name])
            newRow << new V8ViewItem(v8event->displayName);

        if (m_fieldShown[TimeInPercent]) {
            newRow << new V8ViewItem(QString::number(v8event->totalPercent,'f',2)+QLatin1String(" %"));
            newRow.last()->setData(QVariant(v8event->totalPercent));
        }

        if (m_fieldShown[TotalTime]) {
            newRow << new V8ViewItem(QmlProfilerBaseModel::formatTime(v8event->totalTime));
            newRow.last()->setData(QVariant(v8event->totalTime));
        }

        if (m_fieldShown[SelfTimeInPercent]) {
            newRow << new V8ViewItem(QString::number(v8event->SelfTimeInPercent,'f',2)+QLatin1String(" %"));
            newRow.last()->setData(QVariant(v8event->SelfTimeInPercent));
        }

        if (m_fieldShown[SelfTime]) {
            newRow << new V8ViewItem(QmlProfilerBaseModel::formatTime(v8event->selfTime));
            newRow.last()->setData(QVariant(v8event->selfTime));
        }

        if (m_fieldShown[Details]) {
            newRow << new V8ViewItem(v8event->functionName);
            newRow.last()->setData(QVariant(v8event->functionName));
        }

        if (!newRow.isEmpty()) {
            // no edit
            foreach (QStandardItem *item, newRow)
                item->setEditable(false);

            // metadata
            QStandardItem *firstItem = newRow.at(0);
            firstItem->setData(QVariant(v8event->filename), FilenameRole);
            firstItem->setData(QVariant(v8event->line), LineRole);
            firstItem->setData(QVariant(v8event->typeId), TypeIdRole);

            // append
            m_model->invisibleRootItem()->appendRow(newRow);
        }
    }
}

int QV8ProfilerEventsMainView::selectedTypeId() const
{
    QModelIndex index = selectedItem();
    if (!index.isValid())
        return -1;
    QStandardItem *item = d->m_model->item(index.row(), 0);
    return item->data(TypeIdRole).toInt();
}

void QV8ProfilerEventsMainView::jumpToItem(const QModelIndex &index)
{
    if (d->m_preventSelectBounce)
        return;

    d->m_preventSelectBounce = true;
    QStandardItem *clickedItem = d->m_model->itemFromIndex(index);
    QStandardItem *infoItem;
    if (clickedItem->parent())
        infoItem = clickedItem->parent()->child(clickedItem->row(), 0);
    else
        infoItem = d->m_model->item(index.row(), 0);

    // show in editor
    int line = infoItem->data(LineRole).toInt();
    QString fileName = infoItem->data(FilenameRole).toString();
    if (line!=-1 && !fileName.isEmpty())
        emit gotoSourceLocation(fileName, line, -1);

    // show in callers/callees subwindow
    emit typeSelected(infoItem->data(TypeIdRole).toInt());

    d->m_preventSelectBounce = false;
}

void QV8ProfilerEventsMainView::selectType(int typeId)
{
    for (int i=0; i<d->m_model->rowCount(); i++) {
        QStandardItem *infoItem = d->m_model->item(i, 0);
        if (infoItem->data(TypeIdRole).toInt() == typeId) {
            setCurrentIndex(d->m_model->indexFromItem(infoItem));
            jumpToItem(currentIndex());
            return;
        }
    }
}

void QV8ProfilerEventsMainView::selectEventByLocation(const QString &filename, int line)
{
    if (d->m_preventSelectBounce)
        return;

    for (int i=0; i<d->m_model->rowCount(); i++) {
        QStandardItem *infoItem = d->m_model->item(i, 0);
        if (currentIndex() != d->m_model->indexFromItem(infoItem) &&
                infoItem->data(FilenameRole).toString() == filename &&
                infoItem->data(LineRole).toInt() == line) {
            setCurrentIndex(d->m_model->indexFromItem(infoItem));
            jumpToItem(currentIndex());
            return;
        }
    }
}

QModelIndex QV8ProfilerEventsMainView::selectedItem() const
{
    QModelIndexList sel = selectedIndexes();
    if (sel.isEmpty())
        return QModelIndex();
    else
        return sel.first();
}

QString QV8ProfilerEventsMainView::QV8ProfilerEventsMainViewPrivate::textForItem(QStandardItem *item, bool recursive = true) const
{
    QString str;

    if (recursive) {
        // indentation
        QStandardItem *itemParent = item->parent();
        while (itemParent) {
            str += QLatin1String("    ");
            itemParent = itemParent->parent();
        }
    }

    // item's data
    int colCount = m_model->columnCount();
    for (int j = 0; j < colCount; ++j) {
        QStandardItem *colItem = item->parent() ? item->parent()->child(item->row(),j) : m_model->item(item->row(),j);
        str += colItem->data(Qt::DisplayRole).toString();
        if (j < colCount-1) str += QLatin1Char('\t');
    }
    str += QLatin1Char('\n');

    // recursively print children
    if (recursive && item->child(0))
        for (int j = 0; j != item->rowCount(); j++)
            str += textForItem(item->child(j));

    return str;
}

void QV8ProfilerEventsMainView::copyTableToClipboard() const
{
    QString str;
    // headers
    int columnCount = d->m_model->columnCount();
    for (int i = 0; i < columnCount; ++i) {
        str += d->m_model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        if (i < columnCount - 1)
            str += QLatin1Char('\t');
        else
            str += QLatin1Char('\n');
    }
    // data
    int rowCount = d->m_model->rowCount();
    for (int i = 0; i != rowCount; ++i) {
        str += d->textForItem(d->m_model->item(i));
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

void QV8ProfilerEventsMainView::copyRowToClipboard() const
{
    QString str;
    str = d->textForItem(d->m_model->itemFromIndex(selectedItem()), false);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

////////////////////////////////////////////////////////////////////////////////////

QV8ProfilerEventRelativesView::QV8ProfilerEventRelativesView(QV8ProfilerDataModel *model,
                                                             SubViewType viewType,
                                                             QWidget *parent)
    : QmlProfilerTreeView(parent)
    , m_type(viewType)
    , m_v8Model(model)
    , m_model(new QStandardItemModel(this))
{
    m_model->setSortRole(SortRole);
    setModel(m_model);

    updateHeader();
    setSortingEnabled(false);

    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(jumpToItem(QModelIndex)));
}

QV8ProfilerEventRelativesView::~QV8ProfilerEventRelativesView()
{
}

void QV8ProfilerEventRelativesView::displayType(int index)
{
    QV8ProfilerDataModel::QV8EventData *event = m_v8Model->v8EventDescription(index);
    QTC_CHECK(event);

    QList<QV8ProfilerDataModel::QV8EventSub*> events;
    if (m_type == ParentsView)
        events = event->parentHash.values();
    else
        events = event->childrenHash.values();

    rebuildTree(events);

    updateHeader();
    resizeColumnToContents(0);
    setSortingEnabled(true);
    sortByColumn(1);
}

void QV8ProfilerEventRelativesView::rebuildTree(QList<QV8ProfilerDataModel::QV8EventSub*> events)
{
    clear();

    QStandardItem *topLevelItem = m_model->invisibleRootItem();

    foreach (QV8ProfilerDataModel::QV8EventSub *event, events) {
        QList<QStandardItem *> newRow;
        newRow << new V8ViewItem(event->reference->displayName);
        newRow << new V8ViewItem(QmlProfilerBaseModel::formatTime(event->totalTime));
        newRow << new V8ViewItem(event->reference->functionName);

        newRow.at(0)->setData(QVariant(event->reference->typeId), TypeIdRole);
        newRow.at(0)->setData(QVariant(event->reference->filename), FilenameRole);
        newRow.at(0)->setData(QVariant(event->reference->line), LineRole);
        newRow.at(1)->setData(QVariant(event->totalTime));
        newRow.at(2)->setData(QVariant(event->reference->functionName));

        foreach (QStandardItem *item, newRow)
            item->setEditable(false);

        topLevelItem->appendRow(newRow);
    }
}

void QV8ProfilerEventRelativesView::clear()
{
    m_model->clear();
}

void QV8ProfilerEventRelativesView::updateHeader()
{
    m_model->setColumnCount(3);

    int columnIndex = 0;
    if (m_type == ChildrenView)
        m_model->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(Callee)));
    else
        m_model->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(Caller)));

    m_model->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(TotalTime)));

    if (m_type == ChildrenView)
        m_model->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(CalleeDescription)));
    else
        m_model->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(CallerDescription)));
}

void QV8ProfilerEventRelativesView::jumpToItem(const QModelIndex &index)
{
    QStandardItem *infoItem = m_model->item(index.row(), 0);
    emit typeClicked(infoItem->data(TypeIdRole).toInt());
}

} // namespace Internal
} // namespace QmlProfiler
