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

#include "qmlprofilereventview.h"

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
#include <QMenu>

#include <utils/qtcassert.h>

using namespace QmlDebug;

namespace QmlProfiler {
namespace Internal {

struct Colors {
    Colors () {
        this->bindingLoopBackground = QColor("orange").lighter();
    }

    QColor bindingLoopBackground;
};

Q_GLOBAL_STATIC(Colors, colors)

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

class QmlProfilerEventsWidget::QmlProfilerEventsWidgetPrivate
{
public:
    QmlProfilerEventsWidgetPrivate(QmlProfilerEventsWidget *qq):q(qq) {}
    ~QmlProfilerEventsWidgetPrivate() {}

    QmlProfilerEventsWidget *q;

    Analyzer::IAnalyzerTool *m_profilerTool;
    QmlProfilerViewManager *m_viewContainer;

    QmlProfilerEventsMainView *m_eventTree;
    QmlProfilerEventsParentsAndChildrenView *m_eventChildren;
    QmlProfilerEventsParentsAndChildrenView *m_eventParents;
    QmlProfilerDataModel *m_profilerDataModel;

    bool m_globalStatsEnabled;
};

QmlProfilerEventsWidget::QmlProfilerEventsWidget(QWidget *parent,
                                                 Analyzer::IAnalyzerTool *profilerTool,
                                                 QmlProfilerViewManager *container,
                                                 QmlProfilerDataModel *profilerDataModel )
    : QWidget(parent), d(new QmlProfilerEventsWidgetPrivate(this))
{
    setObjectName(QLatin1String("QmlProfilerEventsView"));

    d->m_profilerDataModel = profilerDataModel;
    connect(d->m_profilerDataModel, SIGNAL(stateChanged()),
            this, SLOT(profilerDataModelStateChanged()));

    d->m_eventTree = new QmlProfilerEventsMainView(QmlProfilerEventsMainView::EventsView, this, d->m_profilerDataModel);
    connect(d->m_eventTree, SIGNAL(gotoSourceLocation(QString,int,int)), this, SIGNAL(gotoSourceLocation(QString,int,int)));
    connect(d->m_eventTree, SIGNAL(showEventInTimeline(int)), this, SIGNAL(showEventInTimeline(int)));

    d->m_eventChildren = new QmlProfilerEventsParentsAndChildrenView(
                QmlProfilerEventsParentsAndChildrenView::ChildrenView,
                this,
                d->m_profilerDataModel);
    d->m_eventParents = new QmlProfilerEventsParentsAndChildrenView(
                QmlProfilerEventsParentsAndChildrenView::ParentsView,
                this,
                d->m_profilerDataModel);
    connect(d->m_eventTree, SIGNAL(eventSelected(int)), d->m_eventChildren, SLOT(displayEvent(int)));
    connect(d->m_eventTree, SIGNAL(eventSelected(int)), d->m_eventParents, SLOT(displayEvent(int)));
    connect(d->m_eventChildren, SIGNAL(eventClicked(int)), d->m_eventTree, SLOT(selectEvent(int)));
    connect(d->m_eventParents, SIGNAL(eventClicked(int)), d->m_eventTree, SLOT(selectEvent(int)));

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
    d->m_globalStatsEnabled = true;
}

QmlProfilerEventsWidget::~QmlProfilerEventsWidget()
{
    delete d;
}

void QmlProfilerEventsWidget::profilerDataModelStateChanged()
{
    if (d->m_profilerDataModel) {
        QmlProfilerDataModel::State newState = d->m_profilerDataModel->currentState();
        if (newState == QmlProfilerDataModel::Empty)
            clear();
    }
}

void QmlProfilerEventsWidget::switchToV8View()
{
    setObjectName(QLatin1String("QmlProfilerV8ProfileView"));
    d->m_eventTree->setViewType(QmlProfilerEventsMainView::V8ProfileView);
    d->m_eventParents->setViewType(QmlProfilerEventsParentsAndChildrenView::V8ParentsView);
    d->m_eventChildren->setViewType(QmlProfilerEventsParentsAndChildrenView::V8ChildrenView);
    setToolTip(tr("Trace information from the v8 JavaScript engine. Available only in Qt5 based applications."));
}

void QmlProfilerEventsWidget::clear()
{
    d->m_eventTree->clear();
    d->m_eventChildren->clear();
    d->m_eventParents->clear();
}

void QmlProfilerEventsWidget::getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd)
{
    clear();
    d->m_eventTree->getStatisticsInRange(rangeStart, rangeEnd);
    d->m_globalStatsEnabled = d->m_eventTree->isRangeGlobal(rangeStart, rangeEnd);
}

QModelIndex QmlProfilerEventsWidget::selectedItem() const
{
    return d->m_eventTree->selectedItem();
}

void QmlProfilerEventsWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    QTC_ASSERT(d->m_viewContainer, return;);

    QMenu menu;
    QAction *copyRowAction = 0;
    QAction *copyTableAction = 0;
    QAction *showExtendedStatsAction = 0;
    QAction *getLocalStatsAction = 0;
    QAction *getGlobalStatsAction = 0;

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
            copyRowAction = menu.addAction(tr("Copy Row"));
        copyTableAction = menu.addAction(tr("Copy Table"));

        if (isQml()) {
            // only for qml events view, not for v8
            showExtendedStatsAction = menu.addAction(tr("Extended Event Statistics"));
            showExtendedStatsAction->setCheckable(true);
            showExtendedStatsAction->setChecked(showExtendedStatistics());
        }
    }

    if (isQml()) {
        menu.addSeparator();
        getLocalStatsAction = menu.addAction(tr("Limit Events Pane to Current Range"));
        if (!d->m_viewContainer->hasValidSelection())
            getLocalStatsAction->setEnabled(false);
        getGlobalStatsAction = menu.addAction(tr("Reset Events Pane"));
        if (hasGlobalStats())
            getGlobalStatsAction->setEnabled(false);
    }

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == copyRowAction)
            copyRowToClipboard();
        if (selectedAction == copyTableAction)
            copyTableToClipboard();
        if (selectedAction == getLocalStatsAction) {
            getStatisticsInRange(d->m_viewContainer->selectionStart(),
                                 d->m_viewContainer->selectionEnd());
        }
        if (selectedAction == getGlobalStatsAction) {
            if (d->m_profilerDataModel) {
                getStatisticsInRange(d->m_profilerDataModel->traceStartTime(),
                                     d->m_profilerDataModel->traceEndTime());
            }
        }
        if (selectedAction == showExtendedStatsAction)
            setShowExtendedStatistics(!showExtendedStatistics());
    }
}

void QmlProfilerEventsWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    emit resized();
}

bool QmlProfilerEventsWidget::mouseOnTable(const QPoint &position) const
{
    QPoint tableTopLeft = d->m_eventTree->mapToGlobal(QPoint(0,0));
    QPoint tableBottomRight = d->m_eventTree->mapToGlobal(QPoint(d->m_eventTree->width(), d->m_eventTree->height()));
    return (position.x() >= tableTopLeft.x() && position.x() <= tableBottomRight.x() && position.y() >= tableTopLeft.y() && position.y() <= tableBottomRight.y());
}

void QmlProfilerEventsWidget::copyTableToClipboard() const
{
    d->m_eventTree->copyTableToClipboard();
}

void QmlProfilerEventsWidget::copyRowToClipboard() const
{
    d->m_eventTree->copyRowToClipboard();
}

void QmlProfilerEventsWidget::updateSelectedEvent(int eventId) const
{
    if (d->m_eventTree->selectedEventId() != eventId)
        d->m_eventTree->selectEvent(eventId);
}

void QmlProfilerEventsWidget::selectBySourceLocation(const QString &filename, int line, int column)
{
    // This slot is used to connect the javascript pane with the qml events pane
    // Our javascript trace data does not store column information
    // thus we ignore it here
    Q_UNUSED(column);
    d->m_eventTree->selectEventByLocation(filename, line);
}

bool QmlProfilerEventsWidget::hasGlobalStats() const
{
    return d->m_globalStatsEnabled;
}

void QmlProfilerEventsWidget::setShowExtendedStatistics(bool show)
{
    d->m_eventTree->setShowExtendedStatistics(show);
}

bool QmlProfilerEventsWidget::showExtendedStatistics() const
{
    return d->m_eventTree->showExtendedStatistics();
}

bool QmlProfilerEventsWidget::isQml() const
{
    return d->m_eventTree->viewType() == QmlProfilerEventsMainView::EventsView;
}
bool QmlProfilerEventsWidget::isV8() const
{
    return d->m_eventTree->viewType() == QmlProfilerEventsMainView::V8ProfileView;
}

////////////////////////////////////////////////////////////////////////////////////

class QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate
{
public:
    QmlProfilerEventsMainViewPrivate(QmlProfilerEventsMainView *qq) : q(qq) {}

    void buildModelFromList(const QList<QmlRangeEventData *> &list, QStandardItem *parentItem );
    void buildV8ModelFromList( const QList<QV8EventData *> &list );
    int getFieldCount();

    QString textForItem(QStandardItem *item, bool recursive) const;


    QmlProfilerEventsMainView *q;

    QmlProfilerEventsMainView::ViewTypes m_viewType;
    QmlProfilerDataModel *m_profilerDataModel;
    QStandardItemModel *m_model;
    QList<bool> m_fieldShown;
    QHash<int, int> m_columnIndex; // maps field enum to column index
    bool m_showExtendedStatistics;
    int m_firstNumericColumn;
    bool m_preventSelectBounce;
};


////////////////////////////////////////////////////////////////////////////////////

QmlProfilerEventsMainView::QmlProfilerEventsMainView(ViewTypes viewType,
                                                     QWidget *parent,
                                                     QmlProfilerDataModel *dataModel)
    : QTreeView(parent), d(new QmlProfilerEventsMainViewPrivate(this))
{
    setObjectName(QLatin1String("QmlProfilerEventsTable"));
    header()->setResizeMode(QHeaderView::Interactive);
    header()->setDefaultSectionSize(100);
    header()->setMinimumSectionSize(50);
    setSortingEnabled(false);
    setFrameStyle(QFrame::NoFrame);

    d->m_model = new QStandardItemModel(this);
    setModel(d->m_model);
    connect(this,SIGNAL(clicked(QModelIndex)), this,SLOT(jumpToItem(QModelIndex)));

    d->m_profilerDataModel = dataModel;
    connect(d->m_profilerDataModel,SIGNAL(stateChanged()),
            this,SLOT(profilerDataModelStateChanged()));
    connect(d->m_profilerDataModel,SIGNAL(detailsChanged(int,QString)),
            this,SLOT(changeDetailsForEvent(int,QString)));

    d->m_firstNumericColumn = 0;
    d->m_preventSelectBounce = false;
    d->m_showExtendedStatistics = false;

    setViewType(viewType);
}

QmlProfilerEventsMainView::~QmlProfilerEventsMainView()
{
    clear();
    delete d->m_model;
    delete d;
}

void QmlProfilerEventsMainView::profilerDataModelStateChanged()
{
    if (d->m_profilerDataModel) {
        QmlProfilerDataModel::State newState = d->m_profilerDataModel->currentState();
        if (newState == QmlProfilerDataModel::Done)
            buildModel();
    }
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

QmlProfilerEventsMainView::ViewTypes QmlProfilerEventsMainView::viewType() const
{
    return d->m_viewType;
}

void QmlProfilerEventsMainView::setViewType(ViewTypes type)
{
    d->m_viewType = type;
    switch (type) {
    case EventsView: {
        setObjectName(QLatin1String("QmlProfilerEventsTable"));
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
        setObjectName(QLatin1String("QmlProfilerV8ProfileTable"));
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

    d->m_columnIndex.clear();
    if (d->m_fieldShown[Name]) {
        d->m_columnIndex[Name] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Location")));
        d->m_firstNumericColumn++;
    }
    if (d->m_fieldShown[Type]) {
        d->m_columnIndex[Type] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Type")));
        d->m_firstNumericColumn++;
    }
    if (d->m_fieldShown[Percent]) {
        d->m_columnIndex[Percent] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Time in Percent")));
    }
    if (d->m_fieldShown[TotalDuration]) {
        d->m_columnIndex[TotalDuration] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Total Time")));
    }
    if (d->m_fieldShown[SelfPercent]) {
        d->m_columnIndex[Type] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Self Time in Percent")));
    }
    if (d->m_fieldShown[SelfDuration]) {
        d->m_columnIndex[SelfDuration] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Self Time")));
    }
    if (d->m_fieldShown[CallCount]) {
        d->m_columnIndex[CallCount] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Calls")));
    }
    if (d->m_fieldShown[TimePerCall]) {
        d->m_columnIndex[TimePerCall] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Mean Time")));
    }
    if (d->m_fieldShown[MedianTime]) {
        d->m_columnIndex[MedianTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Median Time")));
    }
    if (d->m_fieldShown[MaxTime]) {
        d->m_columnIndex[MaxTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Longest Time")));
    }
    if (d->m_fieldShown[MinTime]) {
        d->m_columnIndex[MinTime] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Shortest Time")));
    }
    if (d->m_fieldShown[Details]) {
        d->m_columnIndex[Details] = fieldIndex;
        d->m_model->setHeaderData(fieldIndex++, Qt::Horizontal, QVariant(tr("Details")));
    }
}

void QmlProfilerEventsMainView::setShowExtendedStatistics(bool show)
{
    // Not checking if already set because we don't want the first call to skip
    d->m_showExtendedStatistics = show;
    if (show) {
        if (d->m_fieldShown[MedianTime])
            showColumn(d->m_columnIndex[MedianTime]);
        if (d->m_fieldShown[MaxTime])
            showColumn(d->m_columnIndex[MaxTime]);
        if (d->m_fieldShown[MinTime])
            showColumn(d->m_columnIndex[MinTime]);
    } else{
        if (d->m_fieldShown[MedianTime])
            hideColumn(d->m_columnIndex[MedianTime]);
        if (d->m_fieldShown[MaxTime])
            hideColumn(d->m_columnIndex[MaxTime]);
        if (d->m_fieldShown[MinTime])
            hideColumn(d->m_columnIndex[MinTime]);
    }
}

bool QmlProfilerEventsMainView::showExtendedStatistics() const
{
    return d->m_showExtendedStatistics;
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
    if (d->m_profilerDataModel) {
        clear();
        if (d->m_viewType == V8ProfileView)
            d->buildV8ModelFromList( d->m_profilerDataModel->getV8Events() );
        else
            d->buildModelFromList( d->m_profilerDataModel->getEventDescriptions(), d->m_model->invisibleRootItem() );

        setShowExtendedStatistics(d->m_showExtendedStatistics);

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

void QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::buildModelFromList( const QList<QmlRangeEventData *> &list, QStandardItem *parentItem)
{
    foreach (QmlRangeEventData *binding, list) {
        if (binding->calls == 0)
            continue;

        QList<QStandardItem *> newRow;
        if (m_fieldShown[Name])
            newRow << new EventsViewItem(binding->displayName);

        if (m_fieldShown[Type]) {
            QString typeString = QmlProfilerEventsMainView::nameForType(binding->eventType);
            QString toolTipText;
            if (binding->eventType == Binding) {
                if (binding->bindingType == (int)OptimizedBinding) {
                    typeString = typeString + tr(" (Opt)");
                    toolTipText = tr("Binding is evaluated by the optimized engine.");
                } else if (binding->bindingType == (int)V8Binding) {
                    toolTipText = tr("Binding not optimized (e.g. has side effects or assignments,\n"
                                     "references to elements in other files, loops, etc.)");

                }
            }
            newRow << new EventsViewItem(typeString);
            newRow.last()->setData(QVariant(typeString));
            if (!toolTipText.isEmpty())
                newRow.last()->setToolTip(toolTipText);
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
            newRow.at(0)->setData(QVariant(binding->location.filename),FilenameRole);
            newRow.at(0)->setData(QVariant(binding->location.line),LineRole);
            newRow.at(0)->setData(QVariant(binding->location.column),ColumnRole);
            newRow.at(0)->setData(QVariant(binding->eventId),EventIdRole);
            if (binding->isBindingLoop)
                foreach (QStandardItem *item, newRow) {
                    item->setBackground(colors()->bindingLoopBackground);
                    item->setToolTip(tr("Binding loop detected."));
                }

            // append
            parentItem->appendRow(newRow);
        }
    }
}

void QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::buildV8ModelFromList(const QList<QV8EventData *> &list)
{
    for (int index = 0; index < list.count(); index++) {
        QV8EventData *v8event = list.at(index);
        QList<QStandardItem *> newRow;

        if (m_fieldShown[Name])
            newRow << new EventsViewItem(v8event->displayName);

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
            newRow.at(0)->setData(QString::fromLatin1("%1:%2").arg(v8event->filename, QString::number(v8event->line)), EventHashStrRole);
            newRow.at(0)->setData(QVariant(v8event->filename), FilenameRole);
            newRow.at(0)->setData(QVariant(v8event->line), LineRole);
            newRow.at(0)->setData(QVariant(0),ColumnRole); // v8 events have no column info
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
    if (d->m_profilerDataModel)
        d->m_profilerDataModel->compileStatistics(rangeStart, rangeEnd);
    buildModel();
}

bool QmlProfilerEventsMainView::isRangeGlobal(qint64 rangeStart, qint64 rangeEnd) const
{
    if (d->m_profilerDataModel)
        return d->m_profilerDataModel->traceStartTime() == rangeStart && d->m_profilerDataModel->traceEndTime() == rangeEnd;
    else
        return true;
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
    int column = infoItem->data(ColumnRole).toInt();
    QString fileName = infoItem->data(FilenameRole).toString();
    if (line!=-1 && !fileName.isEmpty())
        emit gotoSourceLocation(fileName, line, column);

    // show in callers/callees subwindow
    emit eventSelected(infoItem->data(EventIdRole).toInt());

    // show in timelinerenderer
    if (d->m_viewType == EventsView)
        emit showEventInTimeline(infoItem->data(EventIdRole).toInt());

    d->m_preventSelectBounce = false;
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
    if (d->m_preventSelectBounce)
        return;

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

void QmlProfilerEventsMainView::changeDetailsForEvent(int eventId, const QString &newString)
{
    // available only for QML
    if (d->m_viewType != EventsView)
        return;

    for (int i=0; i<d->m_model->rowCount(); i++) {
        QStandardItem *infoItem = d->m_model->item(i, 0);
        if (infoItem->data(EventIdRole).toInt() == eventId) {
            d->m_model->item(i,d->m_columnIndex[Details])->setData(QVariant(newString),Qt::DisplayRole);
            d->m_model->item(i,d->m_columnIndex[Details])->setData(QVariant(newString));
            return;
        }
    }
}

QString QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::textForItem(QStandardItem *item, bool recursive = true) const
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

void QmlProfilerEventsMainView::copyTableToClipboard() const
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

void QmlProfilerEventsMainView::copyRowToClipboard() const
{
    QString str;
    str = d->textForItem(d->m_model->itemFromIndex(selectedItem()), false);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

////////////////////////////////////////////////////////////////////////////////////

QmlProfilerEventsParentsAndChildrenView::QmlProfilerEventsParentsAndChildrenView(
        SubViewType subtableType, QWidget *parent, QmlProfilerDataModel *model)
    : QTreeView(parent)
{
    m_profilerDataModel = model;
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
    if (!m_profilerDataModel)
        return;

    bool isV8 = m_subtableType == V8ParentsView || m_subtableType == V8ChildrenView;
    bool isChildren = m_subtableType == ChildrenView || m_subtableType == V8ChildrenView;

    if (isV8) {
        QV8EventData *v8event = m_profilerDataModel->v8EventDescription(eventId);
        if (v8event) {
            if (isChildren) {
                QList <QV8EventSub *> childrenList = v8event->childrenHash.values();
                rebuildTree((QObject *)&childrenList);
            }
            else {
                QList <QV8EventSub *> parentList = v8event->parentHash.values();
                rebuildTree((QObject *)&parentList);
            }
        }
    } else {
        QmlRangeEventData *qmlEvent = m_profilerDataModel->eventDescription(eventId);
        if (qmlEvent) {
            if (isChildren) {
                QList <QmlRangeEventRelative *> childrenList = qmlEvent->childrenHash.values();
                rebuildTree((QObject *)&childrenList);
            }
            else {
                QList <QmlRangeEventRelative *> parentList = qmlEvent->parentHash.values();
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

void QmlProfilerEventsParentsAndChildrenView::rebuildTree(void *profilerDataModel)
{
    Q_ASSERT(treeModel());
    treeModel()->clear();

    QStandardItem *topLevelItem = treeModel()->invisibleRootItem();
    bool isV8 = m_subtableType == V8ParentsView || m_subtableType == V8ChildrenView;

    QList <QmlRangeEventRelative *> *qmlList = static_cast< QList <QmlRangeEventRelative *> *>(profilerDataModel);
    QList <QV8EventSub*> *v8List = static_cast< QList <QV8EventSub *> *>(profilerDataModel);

    int listLength;
    if (!isV8)
        listLength = qmlList->length();
    else
        listLength = v8List->length();

    for (int index=0; index < listLength; index++) {
        QList<QStandardItem *> newRow;
        if (!isV8) {
            QmlRangeEventRelative *event = qmlList->at(index);

            newRow << new EventsViewItem(event->reference->displayName);
            newRow << new EventsViewItem(QmlProfilerEventsMainView::nameForType(event->reference->eventType));
            newRow << new EventsViewItem(QmlProfilerEventsMainView::displayTime(event->duration));
            newRow << new EventsViewItem(QString::number(event->calls));
            newRow << new EventsViewItem(event->reference->details);
            newRow.at(0)->setData(QVariant(event->reference->eventId), EventIdRole);
            newRow.at(2)->setData(QVariant(event->duration));
            newRow.at(3)->setData(QVariant(event->calls));
            if (event->inLoopPath)
                foreach (QStandardItem *item, newRow) {
                    item->setBackground(colors()->bindingLoopBackground);
                    item->setToolTip(tr("Part of binding loop."));
                }
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
