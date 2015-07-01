/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    Colors () : noteBackground(QColor("orange")), defaultBackground(QColor("white")) {}
    QColor noteBackground;
    QColor defaultBackground;
};

struct RootEventType : public QmlProfilerDataModel::QmlEventTypeData {
    RootEventType()
    {
        QString rootEventName = QmlProfilerEventsMainView::tr("<program>");
        displayName = rootEventName;
        location = QmlEventLocation(rootEventName, 1, 1);
        message = MaximumMessage;
        rangeType = MaximumRangeType;
        detailType = -1;
        data = QmlProfilerEventsMainView::tr("Main Program");
    }
};

Q_GLOBAL_STATIC(Colors, colors)
Q_GLOBAL_STATIC(RootEventType, rootEventType)

class EventsViewItem : public QStandardItem
{
public:
    EventsViewItem(const QString &text) : QStandardItem(text) {}

    virtual bool operator<(const QStandardItem &other) const
    {
        if (column() == 0) {
            // first column is special
            int filenameDiff = QUrl(data(FilenameRole).toString()).fileName().compare(
                        QUrl(other.data(FilenameRole).toString()).fileName(), Qt::CaseInsensitive);
            if (filenameDiff != 0)
                return filenameDiff < 0;

            return data(LineRole).toInt() == other.data(LineRole).toInt() ?
                data(ColumnRole).toInt() < other.data(ColumnRole).toInt() :
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

class QmlProfilerEventsWidget::QmlProfilerEventsWidgetPrivate
{
public:
    QmlProfilerEventsWidgetPrivate(QmlProfilerEventsWidget *qq):q(qq) {}
    ~QmlProfilerEventsWidgetPrivate() {}

    QmlProfilerEventsWidget *q;

    QmlProfilerTool *m_profilerTool;
    QmlProfilerViewManager *m_viewContainer;

    QmlProfilerEventsMainView *m_eventTree;
    QmlProfilerEventRelativesView *m_eventChildren;
    QmlProfilerEventRelativesView *m_eventParents;

    QmlProfilerEventsModelProxy *modelProxy;
    qint64 rangeStart;
    qint64 rangeEnd;
};

static void setViewDefaults(Utils::TreeView *view)
{
    view->setFrameStyle(QFrame::NoFrame);
    QHeaderView *header = view->header();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setDefaultSectionSize(100);
    header->setMinimumSectionSize(50);
}

static QString displayHeader(Fields header)
{
    static const char ctxt[] = "QmlProfiler::Internal::QmlProfilerEventsMainView";

    switch (header) {
    case Callee:
        return QCoreApplication::translate(ctxt, "Callee");
    case CalleeDescription:
        return QCoreApplication::translate(ctxt, "Callee Description");
    case Caller:
        return QCoreApplication::translate(ctxt, "Caller");
    case CallerDescription:
        return QCoreApplication::translate(ctxt, "Caller Description");
    case CallCount:
        return QCoreApplication::translate(ctxt, "Calls");
    case Details:
        return QCoreApplication::translate(ctxt, "Details");
    case Location:
        return QCoreApplication::translate(ctxt, "Location");
    case MaxTime:
        return QCoreApplication::translate(ctxt, "Longest Time");
    case TimePerCall:
        return QCoreApplication::translate(ctxt, "Mean Time");
    case SelfTime:
        return QCoreApplication::translate(ctxt, "Self Time");
    case SelfTimeInPercent:
        return QCoreApplication::translate(ctxt, "Self Time in Percent");
    case MinTime:
        return QCoreApplication::translate(ctxt, "Shortest Time");
    case TimeInPercent:
        return QCoreApplication::translate(ctxt, "Time in Percent");
    case TotalTime:
        return QCoreApplication::translate(ctxt, "Total Time");
    case Type:
        return QCoreApplication::translate(ctxt, "Type");
    case MedianTime:
        return QCoreApplication::translate(ctxt, "Median Time");
    default:
        return QString();
    }
}

QmlProfilerEventsWidget::QmlProfilerEventsWidget(QWidget *parent,
                                                 QmlProfilerTool *profilerTool,
                                                 QmlProfilerViewManager *container,
                                                 QmlProfilerModelManager *profilerModelManager )

    : QWidget(parent), d(new QmlProfilerEventsWidgetPrivate(this))
{
    setObjectName(QLatin1String("QmlProfilerEventsView"));

    d->modelProxy = new QmlProfilerEventsModelProxy(profilerModelManager, this);
    connect(profilerModelManager, SIGNAL(stateChanged()),
            this, SLOT(profilerDataModelStateChanged()));

    d->m_eventTree = new QmlProfilerEventsMainView(this, d->modelProxy);
    connect(d->m_eventTree, SIGNAL(gotoSourceLocation(QString,int,int)), this, SIGNAL(gotoSourceLocation(QString,int,int)));
    connect(d->m_eventTree, SIGNAL(typeSelected(int)), this, SIGNAL(typeSelected(int)));

    d->m_eventChildren = new QmlProfilerEventRelativesView(
                new QmlProfilerEventChildrenModelProxy(profilerModelManager, d->modelProxy, this),
                this);
    d->m_eventParents = new QmlProfilerEventRelativesView(
                new QmlProfilerEventParentsModelProxy(profilerModelManager, d->modelProxy, this),
                this);
    connect(d->m_eventTree, SIGNAL(typeSelected(int)), d->m_eventChildren, SLOT(displayType(int)));
    connect(d->m_eventTree, SIGNAL(typeSelected(int)), d->m_eventParents, SLOT(displayType(int)));
    connect(d->m_eventChildren, SIGNAL(typeClicked(int)), d->m_eventTree, SLOT(selectType(int)));
    connect(d->m_eventParents, SIGNAL(typeClicked(int)), d->m_eventTree, SLOT(selectType(int)));

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
    d->rangeStart = d->rangeEnd = -1;
}

QmlProfilerEventsWidget::~QmlProfilerEventsWidget()
{
    delete d->modelProxy;
    delete d;
}

void QmlProfilerEventsWidget::profilerDataModelStateChanged()
{
}

void QmlProfilerEventsWidget::clear()
{
    d->m_eventTree->clear();
    d->m_eventChildren->clear();
    d->m_eventParents->clear();
}

void QmlProfilerEventsWidget::getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd)
{
    d->rangeStart = rangeStart;
    d->rangeEnd = rangeEnd;
    d->modelProxy->limitToRange(rangeStart, rangeEnd);
}

QModelIndex QmlProfilerEventsWidget::selectedModelIndex() const
{
    return d->m_eventTree->selectedModelIndex();
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

    QPoint position = ev->globalPos();

    if (d->m_profilerTool) {
        QList <QAction *> commonActions = d->m_profilerTool->profilerContextMenuActions();
        foreach (QAction *act, commonActions) {
            menu.addAction(act);
        }
    }

    if (mouseOnTable(position)) {
        menu.addSeparator();
        if (selectedModelIndex().isValid())
            copyRowAction = menu.addAction(tr("Copy Row"));
        copyTableAction = menu.addAction(tr("Copy Table"));

        showExtendedStatsAction = menu.addAction(tr("Extended Event Statistics"));
        showExtendedStatsAction->setCheckable(true);
        showExtendedStatsAction->setChecked(showExtendedStatistics());
    }

    menu.addSeparator();
    getLocalStatsAction = menu.addAction(tr("Limit to Current Range"));
    if (!d->m_viewContainer->hasValidSelection())
        getLocalStatsAction->setEnabled(false);
    getGlobalStatsAction = menu.addAction(tr("Show Full Range"));
    if (hasGlobalStats())
        getGlobalStatsAction->setEnabled(false);

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
        if (selectedAction == getGlobalStatsAction)
            getStatisticsInRange(-1, -1);
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

void QmlProfilerEventsWidget::selectByTypeId(int typeIndex) const
{
    if (d->m_eventTree->selectedTypeId() != typeIndex)
        d->m_eventTree->selectType(typeIndex);
}

void QmlProfilerEventsWidget::selectBySourceLocation(const QString &filename, int line, int column)
{
    d->m_eventTree->selectByLocation(filename, line, column);
}

void QmlProfilerEventsWidget::onVisibleFeaturesChanged(quint64 features)
{
    for (int i = 0; i < MaximumRangeType; ++i) {
        RangeType range = static_cast<RangeType>(i);
        quint64 featureFlag = 1ULL << featureFromRangeType(range);
        if (QmlDebug::Constants::QML_JS_RANGE_FEATURES & featureFlag)
            d->modelProxy->setEventTypeAccepted(range, features & featureFlag);
    }
    d->modelProxy->limitToRange(d->rangeStart, d->rangeEnd);
}

bool QmlProfilerEventsWidget::hasGlobalStats() const
{
    return d->rangeStart == -1 && d->rangeEnd == -1;
}

void QmlProfilerEventsWidget::setShowExtendedStatistics(bool show)
{
    d->m_eventTree->setShowExtendedStatistics(show);
}

bool QmlProfilerEventsWidget::showExtendedStatistics() const
{
    return d->m_eventTree->showExtendedStatistics();
}

class QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate
{
public:
    QmlProfilerEventsMainViewPrivate(QmlProfilerEventsMainView *qq) : q(qq) {}

    int getFieldCount();

    QString textForItem(QStandardItem *item, bool recursive = false) const;


    QmlProfilerEventsMainView *q;

    QmlProfilerEventsModelProxy *modelProxy;
    QStandardItemModel *m_model;
    QList<bool> m_fieldShown;
    QHash<int, int> m_columnIndex; // maps field enum to column index
    bool m_showExtendedStatistics;
    int m_firstNumericColumn;
    bool m_preventSelectBounce;
};

QmlProfilerEventsMainView::QmlProfilerEventsMainView(QWidget *parent,
                                                     QmlProfilerEventsModelProxy *modelProxy) :
    Utils::TreeView(parent), d(new QmlProfilerEventsMainViewPrivate(this))
{
    setViewDefaults(this);
    setObjectName(QLatin1String("QmlProfilerEventsTable"));

    setSortingEnabled(false);

    d->m_model = new QStandardItemModel(this);
    d->m_model->setSortRole(SortRole);
    setModel(d->m_model);
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(jumpToItem(QModelIndex)));

    d->modelProxy = modelProxy;
    connect(d->modelProxy,SIGNAL(dataAvailable()), this, SLOT(buildModel()));
    connect(d->modelProxy,SIGNAL(notesAvailable(int)), this, SLOT(updateNotes(int)));
    d->m_firstNumericColumn = 0;
    d->m_preventSelectBounce = false;
    d->m_showExtendedStatistics = false;

    setFieldViewable(Name, true);
    setFieldViewable(Type, true);
    setFieldViewable(TimeInPercent, true);
    setFieldViewable(TotalTime, true);
    setFieldViewable(SelfTimeInPercent, false);
    setFieldViewable(SelfTime, false);
    setFieldViewable(CallCount, true);
    setFieldViewable(TimePerCall, true);
    setFieldViewable(MaxTime, true);
    setFieldViewable(MinTime, true);
    setFieldViewable(MedianTime, true);
    setFieldViewable(Details, true);

    buildModel();
}

QmlProfilerEventsMainView::~QmlProfilerEventsMainView()
{
    clear();
    delete d->m_model;
    delete d;
}

void QmlProfilerEventsMainView::profilerDataModelStateChanged()
{
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


void QmlProfilerEventsMainView::setHeaderLabels()
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
    clear();
    parseModelProxy();
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

void QmlProfilerEventsMainView::updateNotes(int typeIndex)
{
    const QHash<int, QmlProfilerEventsModelProxy::QmlEventStats> &eventList =
            d->modelProxy->getData();
    const QHash<int, QString> &noteList = d->modelProxy->getNotes();
    QStandardItem *parentItem = d->m_model->invisibleRootItem();

    for (int rowIndex = 0; rowIndex < parentItem->rowCount(); ++rowIndex) {
        int rowType = parentItem->child(rowIndex, 0)->data(TypeIdRole).toInt();
        if (rowType != typeIndex && typeIndex != -1)
            continue;
        const QmlProfilerEventsModelProxy::QmlEventStats &stats = eventList[rowType];

        for (int columnIndex = 0; columnIndex < parentItem->columnCount(); ++columnIndex) {
            QStandardItem *item = parentItem->child(rowIndex, columnIndex);
            QHash<int, QString>::ConstIterator it = noteList.find(rowType);
            if (it != noteList.end()) {
                item->setBackground(colors()->noteBackground);
                item->setToolTip(it.value());
            } else if (stats.isBindingLoop) {
                item->setBackground(colors()->noteBackground);
                item->setToolTip(tr("Binding loop detected."));
            } else if (!item->toolTip().isEmpty()){
                item->setBackground(colors()->defaultBackground);
                item->setToolTip(QString());
            }
        }
    }
}

void QmlProfilerEventsMainView::parseModelProxy()
{
    const QHash<int, QmlProfilerEventsModelProxy::QmlEventStats> &eventList = d->modelProxy->getData();
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &typeList = d->modelProxy->getTypes();


    QHash<int, QmlProfilerEventsModelProxy::QmlEventStats>::ConstIterator it;
    for (it = eventList.constBegin(); it != eventList.constEnd(); ++it) {
        int typeIndex = it.key();
        const QmlProfilerEventsModelProxy::QmlEventStats &stats = it.value();
        const QmlProfilerDataModel::QmlEventTypeData &event =
                (typeIndex != -1 ? typeList[typeIndex] : *rootEventType());
        QStandardItem *parentItem = d->m_model->invisibleRootItem();
        QList<QStandardItem *> newRow;

        if (d->m_fieldShown[Name])
            newRow << new EventsViewItem(event.displayName.isEmpty() ? tr("<bytecode>") :
                                                                       event.displayName);

        if (d->m_fieldShown[Type]) {
            QString typeString = QmlProfilerEventsMainView::nameForType(event.rangeType);
            QString toolTipText;
            if (event.rangeType == Binding) {
                if (event.detailType == (int)OptimizedBinding) {
                    typeString = typeString + QLatin1Char(' ') +  tr("(Opt)");
                    toolTipText = tr("Binding is evaluated by the optimized engine.");
                } else if (event.detailType == (int)V8Binding) {
                    toolTipText = tr("Binding not optimized (might have side effects or assignments,\n"
                                     "references to elements in other files, loops, and so on.)");

                }
            }
            newRow << new EventsViewItem(typeString);
            newRow.last()->setData(QVariant(typeString));
            if (!toolTipText.isEmpty())
                newRow.last()->setToolTip(toolTipText);
        }

        if (d->m_fieldShown[TimeInPercent]) {
            newRow << new EventsViewItem(QString::number(stats.percentOfTime,'f',2)+QLatin1String(" %"));
            newRow.last()->setData(QVariant(stats.percentOfTime));
        }

        if (d->m_fieldShown[TotalTime]) {
            newRow << new EventsViewItem(QmlProfilerBaseModel::formatTime(stats.duration));
            newRow.last()->setData(QVariant(stats.duration));
        }

        if (d->m_fieldShown[CallCount]) {
            newRow << new EventsViewItem(QString::number(stats.calls));
            newRow.last()->setData(QVariant(stats.calls));
        }

        if (d->m_fieldShown[TimePerCall]) {
            newRow << new EventsViewItem(QmlProfilerBaseModel::formatTime(stats.timePerCall));
            newRow.last()->setData(QVariant(stats.timePerCall));
        }

        if (d->m_fieldShown[MedianTime]) {
            newRow << new EventsViewItem(QmlProfilerBaseModel::formatTime(stats.medianTime));
            newRow.last()->setData(QVariant(stats.medianTime));
        }

        if (d->m_fieldShown[MaxTime]) {
            newRow << new EventsViewItem(QmlProfilerBaseModel::formatTime(stats.maxTime));
            newRow.last()->setData(QVariant(stats.maxTime));
        }

        if (d->m_fieldShown[MinTime]) {
            newRow << new EventsViewItem(QmlProfilerBaseModel::formatTime(stats.minTime));
            newRow.last()->setData(QVariant(stats.minTime));
        }

        if (d->m_fieldShown[Details]) {
            newRow << new EventsViewItem(event.data.isEmpty() ? tr("Source code not available") :
                                                                event.data);
            newRow.last()->setData(QVariant(event.data));
        }



        if (!newRow.isEmpty()) {
            // no edit
            foreach (QStandardItem *item, newRow)
                item->setEditable(false);

            // metadata
            newRow.at(0)->setData(QVariant(typeIndex),TypeIdRole);
            newRow.at(0)->setData(QVariant(event.location.filename),FilenameRole);
            newRow.at(0)->setData(QVariant(event.location.line),LineRole);
            newRow.at(0)->setData(QVariant(event.location.column),ColumnRole);

            // append
            parentItem->appendRow(newRow);
        }
    }
}

QString QmlProfilerEventsMainView::nameForType(RangeType typeNumber)
{
    switch (typeNumber) {
    case Painting: return QmlProfilerEventsMainView::tr("Paint");
    case Compiling: return QmlProfilerEventsMainView::tr("Compile");
    case Creating: return QmlProfilerEventsMainView::tr("Create");
    case Binding: return QmlProfilerEventsMainView::tr("Binding");
    case HandlingSignal: return QmlProfilerEventsMainView::tr("Signal");
    case Javascript: return QmlProfilerEventsMainView::tr("JavaScript");
    default: return QString();
    }
}

void QmlProfilerEventsMainView::getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd)
{
    d->modelProxy->limitToRange(rangeStart, rangeEnd);
}

int QmlProfilerEventsMainView::selectedTypeId() const
{
    QModelIndex index = selectedModelIndex();
    if (!index.isValid())
        return -1;
    QStandardItem *item = d->m_model->item(index.row(), 0);
    return item->data(TypeIdRole).toInt();
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
    emit typeSelected(infoItem->data(TypeIdRole).toInt());

    d->m_preventSelectBounce = false;
}

void QmlProfilerEventsMainView::selectItem(const QStandardItem *item)
{
    // If the same item is already selected, don't reselect it.
    QModelIndex index = d->m_model->indexFromItem(item);
    if (index != currentIndex()) {
        setCurrentIndex(index);
        jumpToItem(index);
    }
}

void QmlProfilerEventsMainView::selectType(int typeIndex)
{
    for (int i=0; i<d->m_model->rowCount(); i++) {
        QStandardItem *infoItem = d->m_model->item(i, 0);
        if (infoItem->data(TypeIdRole).toInt() == typeIndex) {
            selectItem(infoItem);
            return;
        }
    }
}

void QmlProfilerEventsMainView::selectByLocation(const QString &filename, int line, int column)
{
    if (d->m_preventSelectBounce)
        return;

    for (int i=0; i<d->m_model->rowCount(); i++) {
        QStandardItem *infoItem = d->m_model->item(i, 0);
        if (infoItem->data(FilenameRole).toString() == filename &&
                infoItem->data(LineRole).toInt() == line &&
                (column == -1 ||
                infoItem->data(ColumnRole).toInt() == column)) {
            selectItem(infoItem);
            return;
        }
    }
}

QModelIndex QmlProfilerEventsMainView::selectedModelIndex() const
{
    QModelIndexList sel = selectedIndexes();
    if (sel.isEmpty())
        return QModelIndex();
    else
        return sel.first();
}

QString QmlProfilerEventsMainView::QmlProfilerEventsMainViewPrivate::textForItem(QStandardItem *item, bool recursive) const
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
    str = d->textForItem(d->m_model->itemFromIndex(selectedModelIndex()), false);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

class QmlProfilerEventRelativesView::QmlProfilerEventParentsViewPrivate
{
public:
    QmlProfilerEventParentsViewPrivate(QmlProfilerEventRelativesView *qq):q(qq) {}
    ~QmlProfilerEventParentsViewPrivate() {}

    QmlProfilerEventRelativesModelProxy *modelProxy;

    QmlProfilerEventRelativesView *q;
};

QmlProfilerEventRelativesView::QmlProfilerEventRelativesView(
        QmlProfilerEventRelativesModelProxy *modelProxy, QWidget *parent) :
    Utils::TreeView(parent), d(new QmlProfilerEventParentsViewPrivate(this))
{
    setViewDefaults(this);
    setSortingEnabled(false);
    d->modelProxy = modelProxy;
    QStandardItemModel *model = new QStandardItemModel(this);
    model->setSortRole(SortRole);
    setModel(model);
    setRootIsDecorated(false);
    updateHeader();

    connect(this,SIGNAL(activated(QModelIndex)), this,SLOT(jumpToItem(QModelIndex)));

    // Clear when new data available as the selection may be invalid now.
    connect(d->modelProxy, SIGNAL(dataAvailable()), this, SLOT(clear()));
}

QmlProfilerEventRelativesView::~QmlProfilerEventRelativesView()
{
    delete d;
}

void QmlProfilerEventRelativesView::displayType(int typeIndex)
{
    rebuildTree(d->modelProxy->getData(typeIndex));

    updateHeader();
    resizeColumnToContents(0);
    setSortingEnabled(true);
    sortByColumn(2);
}

void QmlProfilerEventRelativesView::rebuildTree(
        const QmlProfilerEventRelativesModelProxy::QmlEventRelativesMap &eventMap)
{
    Q_ASSERT(treeModel());
    treeModel()->clear();

    QStandardItem *topLevelItem = treeModel()->invisibleRootItem();
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &typeList = d->modelProxy->getTypes();

    QmlProfilerEventRelativesModelProxy::QmlEventRelativesMap::const_iterator it;
    for (it = eventMap.constBegin(); it != eventMap.constEnd(); ++it) {
        const QmlProfilerEventRelativesModelProxy::QmlEventRelativesData &event = it.value();
        int typeIndex = it.key();
        const QmlProfilerDataModel::QmlEventTypeData &type =
                (typeIndex != -1 ? typeList[typeIndex] : *rootEventType());
        QList<QStandardItem *> newRow;

        // ToDo: here we were going to search for the data in the other modelproxy
        // maybe we should store the data in this proxy and get it here
        // no indirections at this level of abstraction!
        newRow << new EventsViewItem(type.displayName.isEmpty() ? tr("<bytecode>") :
                                                                  type.displayName);
        newRow << new EventsViewItem(QmlProfilerEventsMainView::nameForType(type.rangeType));
        newRow << new EventsViewItem(QmlProfilerBaseModel::formatTime(event.duration));
        newRow << new EventsViewItem(QString::number(event.calls));
        newRow << new EventsViewItem(type.data.isEmpty() ? tr("Source code not available") :
                                                           type.data);

        newRow.at(0)->setData(QVariant(typeIndex), TypeIdRole);
        newRow.at(0)->setData(QVariant(type.location.filename),FilenameRole);
        newRow.at(0)->setData(QVariant(type.location.line),LineRole);
        newRow.at(0)->setData(QVariant(type.location.column),ColumnRole);
        newRow.at(1)->setData(QVariant(QmlProfilerEventsMainView::nameForType(type.rangeType)));
        newRow.at(2)->setData(QVariant(event.duration));
        newRow.at(3)->setData(QVariant(event.calls));
        newRow.at(4)->setData(QVariant(type.data));

        if (event.isBindingLoop) {
            foreach (QStandardItem *item, newRow) {
                item->setBackground(colors()->noteBackground);
                item->setToolTip(tr("Part of binding loop."));
            }
        }

        foreach (QStandardItem *item, newRow)
            item->setEditable(false);

        topLevelItem->appendRow(newRow);
    }
}

void QmlProfilerEventRelativesView::clear()
{
    if (treeModel()) {
        treeModel()->clear();
        updateHeader();
    }
}

void QmlProfilerEventRelativesView::updateHeader()
{
    bool calleesView = qobject_cast<QmlProfilerEventChildrenModelProxy *>(d->modelProxy) != 0;

    if (treeModel()) {
        treeModel()->setColumnCount(5);

        int columnIndex = 0;
        if (calleesView)
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(Callee)));
        else
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(Caller)));

        treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(Type)));

        treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(TotalTime)));

        treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(CallCount)));


        if (calleesView)
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(CalleeDescription)));
        else
            treeModel()->setHeaderData(columnIndex++, Qt::Horizontal, QVariant(displayHeader(CallerDescription)));
    }
}

QStandardItemModel *QmlProfilerEventRelativesView::treeModel()
{
    return qobject_cast<QStandardItemModel *>(model());
}

void QmlProfilerEventRelativesView::jumpToItem(const QModelIndex &index)
{
    if (treeModel()) {
        QStandardItem *infoItem = treeModel()->item(index.row(), 0);
        emit typeClicked(infoItem->data(TypeIdRole).toInt());
    }
}

} // namespace Internal
} // namespace QmlProfiler
