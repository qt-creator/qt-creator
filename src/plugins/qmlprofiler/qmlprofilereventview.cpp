/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlprofilereventview.h"

#include <QtCore/QUrl>
#include <QtCore/QHash>

#include <QtGui/QStandardItem>
#include <QtGui/QHeaderView>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>

#include <QtGui/QContextMenuEvent>
#include <QDebug>

#include <coreplugin/minisplitter.h>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>


using namespace QmlJsDebugClient;

namespace QmlProfiler {
namespace Internal {

////////////////////////////////////////////////////////////////////////////////////

class EventsViewItem : public QStandardItem
{
public:
    EventsViewItem(const QString &text) : QStandardItem(text) {}

    virtual bool operator<(const QStandardItem &other) const
    {
        if (data().type() == QVariant::String) {
            // first column
            if (column() == 0) {
                return data(FilenameRole).toString() == other.data(FilenameRole).toString() ?
                        data(LineRole).toInt() < other.data(LineRole).toInt() :
                        data(FilenameRole).toString() < other.data(FilenameRole).toString();
            } else {
                return data().toString().toLower() < other.data().toString().toLower();
            }
        }

        return data().toDouble() < other.data().toDouble();
    }
};

////////////////////////////////////////////////////////////////////////////////////

QmlProfilerEventsWidget::QmlProfilerEventsWidget(QmlJsDebugClient::QmlProfilerEventList *model, QWidget *parent) : QWidget(parent)
{
    setObjectName("QmlProfilerEventsView");

    m_eventTree = new QmlProfilerEventsMainView(model, this);
    m_eventTree->setViewType(QmlProfilerEventsMainView::EventsView);
    connect(m_eventTree, SIGNAL(gotoSourceLocation(QString,int)), this, SIGNAL(gotoSourceLocation(QString,int)));
    connect(m_eventTree, SIGNAL(showEventInTimeline(int)), this, SIGNAL(showEventInTimeline(int)));

    m_eventChildren = new QmlProfilerEventsParentsAndChildrenView(model, QmlProfilerEventsParentsAndChildrenView::ChildrenView, this);
    m_eventParents = new QmlProfilerEventsParentsAndChildrenView(model, QmlProfilerEventsParentsAndChildrenView::ParentsView, this);
    connect(m_eventTree, SIGNAL(eventSelected(int)), m_eventChildren, SLOT(displayEvent(int)));
    connect(m_eventTree, SIGNAL(eventSelected(int)), m_eventParents, SLOT(displayEvent(int)));
    connect(m_eventChildren, SIGNAL(eventClicked(int)), m_eventTree, SLOT(selectEvent(int)));
    connect(m_eventParents, SIGNAL(eventClicked(int)), m_eventTree, SLOT(selectEvent(int)));

    // widget arrangement
    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0,0,0,0);
    groupLayout->setSpacing(0);
    Core::MiniSplitter *splitterVertical = new Core::MiniSplitter;
    splitterVertical->addWidget(m_eventTree);
    Core::MiniSplitter *splitterHorizontal = new Core::MiniSplitter;
    splitterHorizontal->addWidget(m_eventParents);
    splitterHorizontal->addWidget(m_eventChildren);
    splitterHorizontal->setOrientation(Qt::Horizontal);
    splitterVertical->addWidget(splitterHorizontal);
    splitterVertical->setOrientation(Qt::Vertical);
    splitterVertical->setStretchFactor(0,5);
    splitterVertical->setStretchFactor(1,2);
    groupLayout->addWidget(splitterVertical);
    setLayout(groupLayout);

    if (model) {
        connect(model,SIGNAL(dataReady()),m_eventChildren,SLOT(clear()));
        connect(model,SIGNAL(dataReady()),m_eventParents,SLOT(clear()));
    }

    m_globalStatsEnabled = true;
}

QmlProfilerEventsWidget::~QmlProfilerEventsWidget()
{
}

void QmlProfilerEventsWidget::switchToV8View()
{
    setObjectName("QmlProfilerV8ProfileView");
    m_eventTree->setViewType(QmlProfilerEventsMainView::V8ProfileView);
    m_eventParents->setViewType(QmlProfilerEventsParentsAndChildrenView::V8ParentsView);
    m_eventChildren->setViewType(QmlProfilerEventsParentsAndChildrenView::V8ChildrenView);
    setToolTip(tr("Trace information from the v8 JavaScript engine. Available only in Qt5 based applications"));
}

void QmlProfilerEventsWidget::clear()
{
    m_eventTree->clear();
    m_eventChildren->clear();
    m_eventParents->clear();
}

void QmlProfilerEventsWidget::getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd)
{
    clear();
    m_eventTree->getStatisticsInRange(rangeStart, rangeEnd);
    m_globalStatsEnabled = m_eventTree->isRangeGlobal(rangeStart, rangeEnd);
}

QModelIndex QmlProfilerEventsWidget::selectedItem() const
{
    return m_eventTree->selectedItem();
}

void QmlProfilerEventsWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    emit contextMenuRequested(ev->globalPos());
}

bool QmlProfilerEventsWidget::mouseOnTable(const QPoint &position) const
{
    QPoint tableTopLeft = m_eventTree->mapToGlobal(QPoint(0,0));
    QPoint tableBottomRight = m_eventTree->mapToGlobal(QPoint(m_eventTree->width(), m_eventTree->height()));
    return (position.x() >= tableTopLeft.x() && position.x() <= tableBottomRight.x() && position.y() >= tableTopLeft.y() && position.y() <= tableBottomRight.y());
}

void QmlProfilerEventsWidget::copyTableToClipboard() const
{
    m_eventTree->copyTableToClipboard();
}

void QmlProfilerEventsWidget::copyRowToClipboard() const
{
    m_eventTree->copyRowToClipboard();
}

void QmlProfilerEventsWidget::updateSelectedEvent(int eventId) const
{
    if (m_eventTree->selectedEventId() != eventId)
        m_eventTree->selectEvent(eventId);
}

void QmlProfilerEventsWidget::selectBySourceLocation(const QString &filename, int line)
{
    m_eventTree->selectEventByLocation(filename, line);
}

bool QmlProfilerEventsWidget::hasGlobalStats() const
{
    return m_globalStatsEnabled;
}

////////////////////////////////////////////////////////////////////////////////////

class QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate
{
public:
    QmlProfilerEventsMainViewPrivate(QmlProfilerEventsMainView *qq) : q(qq) {}

    void buildModelFromList(const QmlEventDescriptions &list, QStandardItem *parentItem, const QmlEventDescriptions &visitedFunctionsList = QmlEventDescriptions() );
    void buildV8ModelFromList( const QV8EventDescriptions &list );
    int getFieldCount();

    QString textForItem(QStandardItem *item, bool recursive) const;


    QmlProfilerEventsMainView *q;

    QmlProfilerEventsMainView::ViewTypes m_viewType;
    QmlProfilerEventList *m_eventStatistics;
    QStandardItemModel *m_model;
    QList<bool> m_fieldShown;
    int m_firstNumericColumn;
};


////////////////////////////////////////////////////////////////////////////////////

QmlProfilerEventsMainView::QmlProfilerEventsMainView(QmlProfilerEventList *model, QWidget *parent) :
    QTreeView(parent), d(new QmlProfilerEventsMainViewPrivate(this))
{
    setObjectName("QmlProfilerEventsTable");
    header()->setResizeMode(QHeaderView::Interactive);
    header()->setDefaultSectionSize(100);
    header()->setMinimumSectionSize(50);
    setSortingEnabled(false);
    setFrameStyle(QFrame::NoFrame);

    d->m_model = new QStandardItemModel(this);
    setModel(d->m_model);
    connect(this,SIGNAL(clicked(QModelIndex)), this,SLOT(jumpToItem(QModelIndex)));

    d->m_eventStatistics = 0;
    setEventStatisticsModel(model);

    d->m_firstNumericColumn = 0;

    // default view
    setViewType(EventsView);
}

QmlProfilerEventsMainView::~QmlProfilerEventsMainView()
{
    clear();
    delete d->m_model;
}

void QmlProfilerEventsMainView::setEventStatisticsModel( QmlProfilerEventList *model )
{
    if (d->m_eventStatistics)
        disconnect(d->m_eventStatistics,SIGNAL(dataReady()),this,SLOT(buildModel()));
    d->m_eventStatistics = model;
    if (model)
        connect(d->m_eventStatistics,SIGNAL(dataReady()),this,SLOT(buildModel()));
}

void QmlProfilerEventsMainView::setFieldViewable(Fields field, bool show)
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

void QmlProfilerEventsMainView::setViewType(ViewTypes type)
{
    d->m_viewType = type;
    switch (type) {
    case EventsView: {
        setObjectName("QmlProfilerEventsTable");
        setFieldViewable(Name, true);
        setFieldViewable(Type, true);
        setFieldViewable(Percent, true);
        setFieldViewable(TotalDuration, true);
        setFieldViewable(SelfPercent, false);
        setFieldViewable(SelfDuration, false);
        setFieldViewable(CallCount, true);
        setFieldViewable(TimePerCall, true);
        setFieldViewable(MaxTime, true);
        setFieldViewable(MinTime, true);
        setFieldViewable(MedianTime, true);
        setFieldViewable(Details, true);
        break;
    }
    case V8ProfileView: {
        setObjectName("QmlProfilerV8ProfileTable");
        setFieldViewable(Name, true);
        setFieldViewable(Type, false);
        setFieldViewable(Percent, true);
        setFieldViewable(TotalDuration, true);
        setFieldViewable(SelfPercent, true);
        setFieldViewable(SelfDuration, true);
        setFieldViewable(CallCount, false);
        setFieldViewable(TimePerCall, false);
        setFieldViewable(MaxTime, false);
        setFieldViewable(MinTime, false);
        setFieldViewable(MedianTime, false);
        setFieldViewable(Details, true);
        break;
    }
    default: break;
    }

    buildModel();
}

void QmlProfilerEventsMainView::setHeaderLabels()
{
    int fieldIndex = 0;
    d->m_firstNumericColumn = 0;

    if (d->m_fieldShown[Name]) {
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Location")));
        d->m_firstNumericColumn++;
    }
    if (d->m_fieldShown[Type]) {
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Type")));
        d->m_firstNumericColumn++;
    }
    if (d->m_fieldShown[Percent])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Time in Percent")));
    if (d->m_fieldShown[TotalDuration])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Total Time")));
    if (d->m_fieldShown[SelfPercent])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Self Time in Percent")));
    if (d->m_fieldShown[SelfDuration])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Self Time")));
    if (d->m_fieldShown[CallCount])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Calls")));
    if (d->m_fieldShown[TimePerCall])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Mean Time")));
    if (d->m_fieldShown[MedianTime])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Median Time")));
    if (d->m_fieldShown[MaxTime])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Longest Time")));
    if (d->m_fieldShown[MinTime])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Shortest Time")));
    if (d->m_fieldShown[Details])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Details")));
}

void QmlProfilerEventsMainView::clear()
{
    d->m_model->clear();
    d->m_model->setColumnCount(d->getFieldCount());

    setHeaderLabels();
    setSortingEnabled(false);
}

int QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::getFieldCount()
{
    int count = 0;
    for (int i=0; i < m_fieldShown.count(); ++i)
        if (m_fieldShown[i])
            count++;
    return count;
}

void QmlProfilerEventsMainView::buildModel()
{
    if (d->m_eventStatistics) {
        clear();
        if (d->m_viewType == V8ProfileView)
            d->buildV8ModelFromList( d->m_eventStatistics->getV8Events() );
        else
            d->buildModelFromList( d->m_eventStatistics->getEventDescriptions(), d->m_model->invisibleRootItem() );

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
}

void QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::buildModelFromList( const QmlEventDescriptions &list, QStandardItem *parentItem, const QmlEventDescriptions &visitedFunctionsList )
{
    foreach (QmlEventData *binding, list) {
        if (visitedFunctionsList.contains(binding))
            continue;

        if (binding->calls == 0)
            continue;

        QList<QStandardItem *> newRow;
        if (m_fieldShown[Name]) {
            newRow << new EventsViewItem(binding->displayname);
        }

        if (m_fieldShown[Type]) {
            newRow << new EventsViewItem(QmlProfilerEventsMainView::nameForType(binding->eventType));
            newRow.last()->setData(QVariant(QmlProfilerEventsMainView::nameForType(binding->eventType)));
        }

        if (m_fieldShown[Percent]) {
            newRow << new EventsViewItem(QString::number(binding->percentOfTime,'f',2)+QLatin1String(" %"));
            newRow.last()->setData(QVariant(binding->percentOfTime));
        }

        if (m_fieldShown[TotalDuration]) {
            newRow << new EventsViewItem(displayTime(binding->duration));
            newRow.last()->setData(QVariant(binding->duration));
        }

        if (m_fieldShown[CallCount]) {
            newRow << new EventsViewItem(QString::number(binding->calls));
            newRow.last()->setData(QVariant(binding->calls));
        }

        if (m_fieldShown[TimePerCall]) {
            newRow << new EventsViewItem(displayTime(binding->timePerCall));
            newRow.last()->setData(QVariant(binding->timePerCall));
        }

        if (m_fieldShown[MedianTime]) {
            newRow << new EventsViewItem(displayTime(binding->medianTime));
            newRow.last()->setData(QVariant(binding->medianTime));
        }

        if (m_fieldShown[MaxTime]) {
            newRow << new EventsViewItem(displayTime(binding->maxTime));
            newRow.last()->setData(QVariant(binding->maxTime));
        }

        if (m_fieldShown[MinTime]) {
            newRow << new EventsViewItem(displayTime(binding->minTime));
            newRow.last()->setData(QVariant(binding->minTime));
        }

        if (m_fieldShown[Details]) {
            newRow << new EventsViewItem(binding->details);
            newRow.last()->setData(QVariant(binding->details));
        }



        if (!newRow.isEmpty()) {
            // no edit
            foreach (QStandardItem *item, newRow)
                item->setEditable(false);

            // metadata
            newRow.at(0)->setData(QVariant(binding->eventHashStr),EventHashStrRole);
            newRow.at(0)->setData(QVariant(binding->filename),FilenameRole);
            newRow.at(0)->setData(QVariant(binding->line),LineRole);
            newRow.at(0)->setData(QVariant(binding->eventId),EventIdRole);

            // append
            parentItem->appendRow(newRow);
        }
    }
}

void QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::buildV8ModelFromList(const QV8EventDescriptions &list)
{
    for (int index = 0; index < list.count(); index++) {
        QV8EventData *v8event = list.at(index);
        QList<QStandardItem *> newRow;

        if (m_fieldShown[Name]) {
            newRow << new EventsViewItem(v8event->displayName);
        }

        if (m_fieldShown[Percent]) {
            newRow << new EventsViewItem(QString::number(v8event->totalPercent,'f',2)+QLatin1String(" %"));
            newRow.last()->setData(QVariant(v8event->totalPercent));
        }

        if (m_fieldShown[TotalDuration]) {
            newRow << new EventsViewItem(displayTime(v8event->totalTime));
            newRow.last()->setData(QVariant(v8event->totalTime));
        }

        if (m_fieldShown[SelfPercent]) {
            newRow << new EventsViewItem(QString::number(v8event->selfPercent,'f',2)+QLatin1String(" %"));
            newRow.last()->setData(QVariant(v8event->selfPercent));
        }

        if (m_fieldShown[SelfDuration]) {
            newRow << new EventsViewItem(displayTime(v8event->selfTime));
            newRow.last()->setData(QVariant(v8event->selfTime));
        }

        if (m_fieldShown[Details]) {
            newRow << new EventsViewItem(v8event->functionName);
            newRow.last()->setData(QVariant(v8event->functionName));
        }

        if (!newRow.isEmpty()) {
            // no edit
            foreach (QStandardItem *item, newRow)
                item->setEditable(false);

            // metadata
            newRow.at(0)->setData(QString("%1:%2").arg(v8event->filename, QString::number(v8event->line)), EventHashStrRole);
            newRow.at(0)->setData(QVariant(v8event->filename), FilenameRole);
            newRow.at(0)->setData(QVariant(v8event->line), LineRole);
            newRow.at(0)->setData(QVariant(v8event->eventId), EventIdRole);

            // append
            m_model->invisibleRootItem()->appendRow(newRow);
        }
    }
}

QString QmlProfilerEventsMainView::displayTime(double time)
{
    if (time < 1e6)
        return QString::number(time/1e3,'f',3) + trUtf8(" \xc2\xb5s");
    if (time < 1e9)
        return QString::number(time/1e6,'f',3) + tr(" ms");

    return QString::number(time/1e9,'f',3) + tr(" s");
}

QString QmlProfilerEventsMainView::nameForType(int typeNumber)
{
    switch (typeNumber) {
    case 0: return QmlProfilerEventsMainView::tr("Paint");
    case 1: return QmlProfilerEventsMainView::tr("Compile");
    case 2: return QmlProfilerEventsMainView::tr("Create");
    case 3: return QmlProfilerEventsMainView::tr("Binding");
    case 4: return QmlProfilerEventsMainView::tr("Signal");
    }
    return QString();
}

void QmlProfilerEventsMainView::getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd)
{
    d->m_eventStatistics->compileStatistics(rangeStart, rangeEnd);
    buildModel();
}

bool QmlProfilerEventsMainView::isRangeGlobal(qint64 rangeStart, qint64 rangeEnd) const
{
    return d->m_eventStatistics->traceStartTime() == rangeStart && d->m_eventStatistics->traceEndTime() == rangeEnd;
}

int QmlProfilerEventsMainView::selectedEventId() const
{
    QModelIndex index = selectedItem();
    if (!index.isValid())
        return -1;
    QStandardItem *item = d->m_model->item(index.row(), 0);
    return item->data(EventIdRole).toInt();
}


void QmlProfilerEventsMainView::jumpToItem(const QModelIndex &index)
{
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
        emit gotoSourceLocation(fileName, line);

    // show in callers/callees subwindow
    emit eventSelected(infoItem->data(EventIdRole).toInt());

    // show in timelineview
    if (d->m_viewType == EventsView) {
        emit showEventInTimeline(infoItem->data(EventIdRole).toInt());
    }
}

void QmlProfilerEventsMainView::selectEvent(int eventId)
{
    for (int i=0; i<d->m_model->rowCount(); i++) {
        QStandardItem *infoItem = d->m_model->item(i, 0);
        if (infoItem->data(EventIdRole).toInt() == eventId) {
            setCurrentIndex(d->m_model->indexFromItem(infoItem));
            jumpToItem(currentIndex());
            return;
        }
    }
}

void QmlProfilerEventsMainView::selectEventByLocation(const QString &filename, int line)
{
    for (int i=0; i<d->m_model->rowCount(); i++) {
        QStandardItem *infoItem = d->m_model->item(i, 0);
        if (currentIndex() != d->m_model->indexFromItem(infoItem) && infoItem->data(FilenameRole).toString() == filename && infoItem->data(LineRole).toInt() == line) {
            setCurrentIndex(d->m_model->indexFromItem(infoItem));
            jumpToItem(currentIndex());
            return;
        }
    }
}

QModelIndex QmlProfilerEventsMainView::selectedItem() const
{
    QModelIndexList sel = selectedIndexes();
    if (sel.isEmpty())
        return QModelIndex();
    else
        return sel.first();
}

QString QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::textForItem(QStandardItem *item, bool recursive = true) const
{
    QString str;

    if (recursive) {
        // indentation
        QStandardItem *itemParent = item->parent();
        while (itemParent) {
            str += "    ";
            itemParent = itemParent->parent();
        }
    }

    // item's data
    int colCount = m_model->columnCount();
    for (int j = 0; j < colCount; ++j) {
        QStandardItem *colItem = item->parent() ? item->parent()->child(item->row(),j) : m_model->item(item->row(),j);
        str += colItem->data(Qt::DisplayRole).toString();
        if (j < colCount-1) str += '\t';
    }
    str += '\n';

    // recursively print children
    if (recursive && item->child(0))
        for (int j = 0; j != item->rowCount(); j++)
            str += textForItem(item->child(j));

    return str;
}

void QmlProfilerEventsMainView::copyTableToClipboard() const
{
    QString str;
    // headers
    int columnCount = d->m_model->columnCount();
    for (int i = 0; i < columnCount; ++i) {
        str += d->m_model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        if (i < columnCount - 1)
            str += '\t';
        else
            str += '\n';
    }
    // data
    int rowCount = d->m_model->rowCount();
    for (int i = 0; i != rowCount; ++i) {
        str += d->textForItem(d->m_model->item(i));
    }
    QClipboard *clipboard = QApplication::clipboard();
#    ifdef Q_WS_X11
    clipboard->setText(str, QClipboard::Selection);
#    endif
    clipboard->setText(str, QClipboard::Clipboard);
}

void QmlProfilerEventsMainView::copyRowToClipboard() const
{
    QString str;
    str = d->textForItem(d->m_model->itemFromIndex(selectedItem()), false);

    QClipboard *clipboard = QApplication::clipboard();
#    ifdef Q_WS_X11
    clipboard->setText(str, QClipboard::Selection);
#    endif
    clipboard->setText(str, QClipboard::Clipboard);
}

////////////////////////////////////////////////////////////////////////////////////

QmlProfilerEventsParentsAndChildrenView::QmlProfilerEventsParentsAndChildrenView(QmlJsDebugClient::QmlProfilerEventList *model, SubViewType subtableType, QWidget *parent):QTreeView(parent)
{
    m_eventList = model;
    setModel(new QStandardItemModel(this));
    setRootIsDecorated(false);
    setFrameStyle(QFrame::NoFrame);
    m_subtableType = subtableType;
    updateHeader();

    connect(this,SIGNAL(clicked(QModelIndex)), this,SLOT(jumpToItem(QModelIndex)));
}

QmlProfilerEventsParentsAndChildrenView::~QmlProfilerEventsParentsAndChildrenView()
{
}

void QmlProfilerEventsParentsAndChildrenView::setViewType(SubViewType type)
{
    m_subtableType = type;
    updateHeader();
}

void QmlProfilerEventsParentsAndChildrenView::displayEvent(int eventId)
{
    bool isV8 = m_subtableType == V8ParentsView || m_subtableType == V8ChildrenView;
    bool isChildren = m_subtableType == ChildrenView || m_subtableType == V8ChildrenView;

    if (isV8) {
        QmlJsDebugClient::QV8EventData *v8event = m_eventList->v8EventDescription(eventId);
        if (v8event) {
            if (isChildren) {
                QList <QmlJsDebugClient::QV8EventSub *> childrenList = v8event->childrenHash.values();
                rebuildTree((QObject *)&childrenList);
            }
            else {
                QList <QmlJsDebugClient::QV8EventSub *> parentList = v8event->parentHash.values();
                rebuildTree((QObject *)&parentList);
            }
        }
    } else {
        QmlJsDebugClient::QmlEventData *qmlEvent = m_eventList->eventDescription(eventId);
        if (qmlEvent) {
            if (isChildren) {
                QList <QmlJsDebugClient::QmlEventSub *> childrenList = qmlEvent->childrenHash.values();
                rebuildTree((QObject *)&childrenList);
            }
            else {
                QList <QmlJsDebugClient::QmlEventSub *> parentList = qmlEvent->parentHash.values();
                rebuildTree((QObject *)&parentList);
            }
        }
    }

    updateHeader();
    resizeColumnToContents(0);
    setSortingEnabled(true);
    if (isV8)
        sortByColumn(1);
    else
        sortByColumn(2);
}

void QmlProfilerEventsParentsAndChildrenView::rebuildTree(void *eventList)
{
    Q_ASSERT(treeModel());
    treeModel()->clear();

    QStandardItem *topLevelItem = treeModel()->invisibleRootItem();
    bool isV8 = m_subtableType == V8ParentsView || m_subtableType == V8ChildrenView;

    QList <QmlEventSub *> *qmlList = static_cast< QList <QmlEventSub *> *>(eventList);
    QList <QV8EventSub*> *v8List = static_cast< QList <QV8EventSub *> *>(eventList);

    int listLength;
    if (!isV8)
        listLength = qmlList->length();
    else
        listLength = v8List->length();

    for (int index=0; index < listLength; index++) {
        QList<QStandardItem *> newRow;
        if (!isV8) {
            QmlEventSub *event = qmlList->at(index);

            newRow << new EventsViewItem(event->reference->displayname);
            newRow << new EventsViewItem(QmlProfilerEventsMainView::nameForType(event->reference->eventType));
            newRow << new EventsViewItem(QmlProfilerEventsMainView::displayTime(event->duration));
            newRow << new EventsViewItem(QString::number(event->calls));
            newRow << new EventsViewItem(event->reference->details);
            newRow.at(0)->setData(QVariant(event->reference->eventId), EventIdRole);
            newRow.at(2)->setData(QVariant(event->duration));
            newRow.at(3)->setData(QVariant(event->calls));
        } else {
            QV8EventSub *event = v8List->at(index);
            newRow << new EventsViewItem(event->reference->displayName);
            newRow << new EventsViewItem(QmlProfilerEventsMainView::displayTime(event->totalTime));
            newRow << new EventsViewItem(event->reference->functionName);
            newRow.at(0)->setData(QVariant(event->reference->eventId), EventIdRole);
            newRow.at(1)->setData(QVariant(event->totalTime));
        }
        foreach (QStandardItem *item, newRow)
            item->setEditable(false);

        topLevelItem->appendRow(newRow);
    }
}

void QmlProfilerEventsParentsAndChildrenView::clear()
{
    if (treeModel()) {
        treeModel()->clear();
        updateHeader();
    }
}

void QmlProfilerEventsParentsAndChildrenView::updateHeader()
{
    bool isV8 = m_subtableType == V8ParentsView || m_subtableType == V8ChildrenView;
    bool isChildren = m_subtableType == ChildrenView || m_subtableType == V8ChildrenView;

    header()->setResizeMode(QHeaderView::Interactive);
    header()->setDefaultSectionSize(100);
    header()->setMinimumSectionSize(50);

    if (treeModel()) {
        if (isV8)
            treeModel()->setColumnCount(3);
        else
            treeModel()->setColumnCount(5);

        int columnIndex = 0;
        if (isChildren)
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(tr("Callee")));
        else
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(tr("Caller")));

        if (!isV8)
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(tr("Type")));

        treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(tr("Total Time")));

        if (!isV8)
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(tr("Calls")));

        if (isChildren)
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(tr("Callee Description")));
        else
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(tr("Caller Description")));
    }
}

QStandardItemModel *QmlProfilerEventsParentsAndChildrenView::treeModel()
{
    return qobject_cast<QStandardItemModel *>(model());
}

void QmlProfilerEventsParentsAndChildrenView::jumpToItem(const QModelIndex &index)
{
    if (treeModel()) {
        QStandardItem *infoItem = treeModel()->item(index.row(), 0);
        emit eventClicked(infoItem->data(EventIdRole).toInt());
    }
}

} // namespace Internal
} // namespace QmlProfiler
