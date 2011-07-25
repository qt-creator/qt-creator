/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilereventview.h"

#include <QtCore/QUrl>
#include <QtCore/QHash>

#include <QtGui/QHeaderView>
#include <QtGui/QStandardItemModel>

namespace QmlProfiler {
namespace Internal {

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
                return data().toString() < other.data().toString();
            }
        }

        return data().toDouble() < other.data().toDouble();
    }
};


////////////////////////////////////////////////////////////////////////////////////

class QmlProfilerEventStatistics::QmlProfilerEventStatisticsPrivate
{
public:
    QmlProfilerEventStatisticsPrivate(QmlProfilerEventStatistics *qq) : q(qq) {}

    void postProcess();

    QmlProfilerEventStatistics *q;
    QmlEventHash m_rootHash;
    QHash<int, QmlEventList> m_pendingEvents;
    int m_lastLevel;
};


////////////////////////////////////////////////////////////////////////////////////

class QmlProfilerEventsView::QmlProfilerEventsViewPrivate
{
public:
    QmlProfilerEventsViewPrivate(QmlProfilerEventsView *qq) : q(qq) {}

    void buildModelFromList(const QmlEventList &list, QStandardItem *parentItem, const QmlEventList &visitedFunctionsList = QmlEventList() );
    int getFieldCount();
    QString displayTime(double time) const;
    QString nameForType(int typeNumber) const;


    QmlProfilerEventsView *q;

    QmlProfilerEventStatistics *m_eventStatistics;
    QStandardItemModel *m_model;
    QList<bool> m_fieldShown;
    bool m_showAnonymous;
    int m_firstNumericColumn;
};


////////////////////////////////////////////////////////////////////////////////////

QmlProfilerEventStatistics::QmlProfilerEventStatistics(QObject *parent) :
    QObject(parent), d(new QmlProfilerEventStatisticsPrivate(this))
{
    setObjectName("QmlProfilerEventStatistics");
    d->m_lastLevel = -1;
}

QmlProfilerEventStatistics::~QmlProfilerEventStatistics()
{
    clear();
}

void QmlProfilerEventStatistics::clear()
{
    foreach (int levelNumber, d->m_pendingEvents.keys())
        d->m_pendingEvents[levelNumber].clear();

    d->m_lastLevel = -1;

    foreach (QmlEventData *binding, d->m_rootHash.values())
        delete binding;
    d->m_rootHash.clear();
}

QList <QmlEventData *> QmlProfilerEventStatistics::getEventList() const
{
    return d->m_rootHash.values();
}

int QmlProfilerEventStatistics::eventCount() const
{
    return d->m_rootHash.size();
}

void QmlProfilerEventStatistics::addRangedEvent(int type, int nestingLevel, int nestingInType, qint64 startTime, qint64 length,
                                                const QStringList &data, const QString &fileName, int line)
{
    Q_UNUSED(startTime);
    Q_UNUSED(nestingInType);

    const QChar colon = QLatin1Char(':');
    QString displayName, location, details;

    if (data.isEmpty())
        details = tr("Source code not available");
    else {
        details = data.join(" ").replace('\n'," ").simplified();
        QRegExp rewrite("\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)");
        bool match = rewrite.exactMatch(details);
        if (match) {
            details = rewrite.cap(1) + ": " + rewrite.cap(3);
        }
        if (details.startsWith(QString("file://")))
            details = details.mid(details.lastIndexOf(QChar('/')) + 1);
    }

    if (fileName.isEmpty()) {
        displayName = tr("<bytecode>");
        location = QString("--:%1:%2").arg(QString::number(type), details);
    } else {
        const QString filePath = QUrl(fileName).path();
        displayName = filePath.mid(filePath.lastIndexOf(QChar('/')) + 1) + colon + QString::number(line);
        location = fileName+colon+QString::number(line);
    }


    // New Data:  if it's not in the hash, put it there
    // if it's in the hash, get the reference from the hash
    QmlEventData *newBinding;
    QmlEventHash::iterator it = d->m_rootHash.find(location);
    if (it != d->m_rootHash.end()) {
        newBinding = it.value();
        newBinding->duration += length;
        newBinding->calls++;
        if (newBinding->maxTime < length)
            newBinding->maxTime = length;
        if (newBinding->minTime > length)
            newBinding->minTime = length;
    } else {
        newBinding = new QmlEventData;
        newBinding->calls = 1;
        newBinding->duration = length;
        newBinding->displayname = new QString(displayName);
        newBinding->filename = new QString(fileName);
        newBinding->location = new QString(location);
        newBinding->line = line;
        newBinding->minTime = length;
        newBinding->maxTime = length;
        newBinding->level = nestingLevel;
        newBinding->eventType = (QmlEventType)type;
        newBinding->details = new QString(details);
        newBinding->parentList = new QmlEventList();
        newBinding->childrenList = new QmlEventList();
        d->m_rootHash.insert(location, newBinding);
    }

    if (nestingLevel < d->m_lastLevel) {
        // I'm the parent of the former
        if (d->m_pendingEvents.contains(nestingLevel+1)) {
            foreach (QmlEventData *child, d->m_pendingEvents[nestingLevel + 1]) {
                if (!newBinding->childrenList->contains(child))
                    newBinding->childrenList->append(child);
                if (!child->parentList->contains(newBinding))
                    child->parentList->append(newBinding);
            }
            d->m_pendingEvents[nestingLevel + 1].clear();
        }

    }

    if (nestingLevel > 1 && !d->m_pendingEvents[nestingLevel].contains(newBinding)) {
        // I'm not root... there will come a parent later
        d->m_pendingEvents[nestingLevel].append(newBinding);
    }

    d->m_lastLevel = nestingLevel;
}

void QmlProfilerEventStatistics::complete()
{
    d->postProcess();
    emit dataReady();
}

void QmlProfilerEventStatistics::QmlProfilerEventStatisticsPrivate::postProcess()
{
    double totalTime = 0;

    foreach (QmlEventData *binding, m_rootHash.values()) {
        if (binding->filename->isEmpty())
            continue;
        totalTime += binding->duration;
    }

    foreach (QmlEventData *binding, m_rootHash.values()) {
        if (binding->filename->isEmpty())
            continue;
        binding->percentOfTime = binding->duration * 100.0 / totalTime;
        binding->timePerCall = binding->calls > 0 ? double(binding->duration) / binding->calls : 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////

QmlProfilerEventsView::QmlProfilerEventsView(QWidget *parent, QmlProfilerEventStatistics *model) :
    QTreeView(parent), d(new QmlProfilerEventsViewPrivate(this))
{
    setObjectName("QmlProfilerEventsView");
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

    d->m_showAnonymous = false;
    d->m_firstNumericColumn = 0;

    // default view
    setViewType(EventsView);
}

QmlProfilerEventsView::~QmlProfilerEventsView()
{
    clear();
    delete d->m_model;
}

void QmlProfilerEventsView::setEventStatisticsModel( QmlProfilerEventStatistics *model )
{
    if (d->m_eventStatistics)
        disconnect(d->m_eventStatistics,SIGNAL(dataReady()),this,SLOT(buildModel()));
    d->m_eventStatistics = model;
    connect(d->m_eventStatistics,SIGNAL(dataReady()),this,SLOT(buildModel()));
}

void QmlProfilerEventsView::setFieldViewable(Fields field, bool show)
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

void QmlProfilerEventsView::setViewType(ViewTypes type)
{
    switch (type) {
    case EventsView: {
        setObjectName("QmlProfilerEventsView");
        setFieldViewable(Name, true);
        setFieldViewable(Type, true);
        setFieldViewable(Percent, true);
        setFieldViewable(TotalDuration, true);
        setFieldViewable(CallCount, true);
        setFieldViewable(TimePerCall, true);
        setFieldViewable(MaxTime, true);
        setFieldViewable(MinTime, true);
        setFieldViewable(Details, false);
        setFieldViewable(Parents, false);
        setFieldViewable(Children, false);
        setShowAnonymousEvents(false);
        break;
    }
    case CallersView: {
        setObjectName("QmlProfilerCallersView");
        setFieldViewable(Name, true);
        setFieldViewable(Type, true);
        setFieldViewable(Percent, false);
        setFieldViewable(TotalDuration, false);
        setFieldViewable(CallCount, false);
        setFieldViewable(TimePerCall, false);
        setFieldViewable(MaxTime, false);
        setFieldViewable(MinTime, false);
        setFieldViewable(Details, true);
        setFieldViewable(Parents, true);
        setFieldViewable(Children, false);
        setShowAnonymousEvents(true);
        break;
    }
    case CalleesView: {
        setObjectName("QmlProfilerCalleesView");
        setFieldViewable(Name, true);
        setFieldViewable(Type, true);
        setFieldViewable(Percent, false);
        setFieldViewable(TotalDuration, false);
        setFieldViewable(CallCount, false);
        setFieldViewable(TimePerCall, false);
        setFieldViewable(MaxTime, false);
        setFieldViewable(MinTime, false);
        setFieldViewable(Details, true);
        setFieldViewable(Parents, false);
        setFieldViewable(Children, true);
        setShowAnonymousEvents(true);
        break;
    }
    default: break;
    }

    buildModel();
}

void QmlProfilerEventsView::setShowAnonymousEvents( bool showThem )
{
    d->m_showAnonymous = showThem;
}

void QmlProfilerEventsView::setHeaderLabels()
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
    if (d->m_fieldShown[CallCount])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Calls")));
    if (d->m_fieldShown[TimePerCall])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Time per Call")));
    if (d->m_fieldShown[MaxTime])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Longest Time")));
    if (d->m_fieldShown[MinTime])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Shortest Time")));
    if (d->m_fieldShown[Details])
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Details")));
}

void QmlProfilerEventsView::clear()
{
    d->m_model->clear();
    d->m_model->setColumnCount(d->getFieldCount());

    setHeaderLabels();
    setSortingEnabled(false);
}

int QmlProfilerEventsView::QmlProfilerEventsViewPrivate::getFieldCount()
{
    int count = 0;
    for (int i=0; i < m_fieldShown.count(); ++i)
        if (m_fieldShown[i] && i != Parents && i != Children)
            count++;
    return count;
}

void QmlProfilerEventsView::buildModel()
{
    if (d->m_eventStatistics) {
        clear();
        d->buildModelFromList( d->m_eventStatistics->getEventList(), d->m_model->invisibleRootItem() );

        bool hasBranches = d->m_fieldShown[Parents] || d->m_fieldShown[Children];
        setRootIsDecorated(hasBranches);

        setSortingEnabled(true);

        if (!hasBranches)
            sortByColumn(d->m_firstNumericColumn,Qt::DescendingOrder);

        expandAll();
        if (d->m_fieldShown[Name])
            resizeColumnToContents(0);

        if (d->m_fieldShown[Type])
            resizeColumnToContents(d->m_fieldShown[Name]?1:0);
        collapseAll();
    }
}

void QmlProfilerEventsView::QmlProfilerEventsViewPrivate::buildModelFromList( const QmlEventList &list, QStandardItem *parentItem, const QmlEventList &visitedFunctionsList )
{
    foreach (QmlEventData *binding, list) {
        if (visitedFunctionsList.contains(binding))
            continue;

        if ((!m_showAnonymous) && binding->filename->isEmpty())
            continue;

        QList<QStandardItem *> newRow;
        if (m_fieldShown[Name]) {
            newRow << new EventsViewItem(*binding->displayname);
        }

        if (m_fieldShown[Type]) {
            newRow << new EventsViewItem(nameForType(binding->eventType));
            newRow.last()->setData(QVariant(binding->eventType));
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

        if (m_fieldShown[MaxTime]) {
            newRow << new EventsViewItem(displayTime(binding->maxTime));
            newRow.last()->setData(QVariant(binding->maxTime));
        }

        if (m_fieldShown[MinTime]) {
            newRow << new EventsViewItem(displayTime(binding->minTime));
            newRow.last()->setData(QVariant(binding->minTime));
        }

        if (m_fieldShown[Details]) {
            newRow << new EventsViewItem(*binding->details);
            newRow.last()->setData(QVariant(*binding->details));
        }

        if (!newRow.isEmpty()) {
            // no edit
            foreach (QStandardItem *item, newRow)
                item->setEditable(false);

            // metadata
            newRow.at(0)->setData(QVariant(*binding->location),LocationRole);
            newRow.at(0)->setData(QVariant(*binding->filename),FilenameRole);
            newRow.at(0)->setData(QVariant(binding->line),LineRole);

            // append
            parentItem->appendRow(newRow);

            if (m_fieldShown[Parents] && !binding->parentList->isEmpty()) {
                QmlEventList newParentList(visitedFunctionsList);
                newParentList.append(binding);

                buildModelFromList(*binding->parentList, newRow.at(0), newParentList);
            }

            if (m_fieldShown[Children] && !binding->childrenList->isEmpty()) {
                QmlEventList newChildrenList(visitedFunctionsList);
                newChildrenList.append(binding);

                buildModelFromList(*binding->childrenList, newRow.at(0), newChildrenList);
            }
        }
    }
}

QString QmlProfilerEventsView::QmlProfilerEventsViewPrivate::displayTime(double time) const
{
    if (time < 1e6)
        return QString::number(time/1e3,'f',3) + QString::fromWCharArray(L" \u03BCs");
    if (time < 1e9)
        return QString::number(time/1e6,'f',3) + QLatin1String(" ms");

    return QString::number(time/1e9,'f',3) + QLatin1String(" s");
}

QString QmlProfilerEventsView::QmlProfilerEventsViewPrivate::nameForType(int typeNumber) const
{
    switch (typeNumber) {
    case 0: return QmlProfilerEventsView::tr("Paint");
    case 1: return QmlProfilerEventsView::tr("Compile");
    case 2: return QmlProfilerEventsView::tr("Create");
    case 3: return QmlProfilerEventsView::tr("Binding");
    case 4: return QmlProfilerEventsView::tr("Signal");
    }
    return QString();
}

void QmlProfilerEventsView::jumpToItem(const QModelIndex &index)
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

} // namespace Internal
} // namespace QmlProfiler
