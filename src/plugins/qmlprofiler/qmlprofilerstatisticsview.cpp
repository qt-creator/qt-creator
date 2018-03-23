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

#include "qmlprofilerstatisticsview.h"
#include "qmlprofilerviewmanager.h"
#include "qmlprofilertool.h"

#include <coreplugin/minisplitter.h>
#include <utils/qtcassert.h>
#include <timeline/timelineformattime.h>

#include <QUrl>
#include <QHash>
#include <QStandardItem>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>

#include <functional>

namespace QmlProfiler {
namespace Internal {

const int DEFAULT_SORT_COLUMN = MainTimeInPercent;

struct SortPreserver {
    SortPreserver(Utils::TreeView *view) : view(view)
    {
        const QHeaderView *header = view->header();
        column = header->sortIndicatorSection();
        order = header->sortIndicatorOrder();
        view->setSortingEnabled(false);
    }

    ~SortPreserver()
    {
        view->setSortingEnabled(true);
        view->sortByColumn(column, order);
    }

    int column = DEFAULT_SORT_COLUMN;
    Qt::SortOrder order = Qt::DescendingOrder;
    Utils::TreeView *view = nullptr;
};

struct Colors {
    QColor noteBackground = QColor("orange");
    QColor defaultBackground = QColor("white");
};

struct RootEventType : public QmlEventType {
    RootEventType() : QmlEventType(MaximumMessage, MaximumRangeType, -1,
                                   QmlEventLocation("<program>", 1, 1),
                                   QmlProfilerStatisticsMainView::tr("Main Program"),
                                   QmlProfilerStatisticsMainView::tr("<program>"))
    {
    }
};

Q_GLOBAL_STATIC(Colors, colors)
Q_GLOBAL_STATIC(RootEventType, rootEventType)

class StatisticsViewItem : public QStandardItem
{
public:
    StatisticsViewItem(const QString &text, const QVariant &sort) : QStandardItem(text)
    {
        setData(sort, SortRole);
    }

    virtual bool operator<(const QStandardItem &other) const
    {
        if (data(SortRole).type() == QVariant::String) {
            // Strings should be case-insensitive compared
            return data(SortRole).toString().compare(other.data(SortRole).toString(),
                                                      Qt::CaseInsensitive) < 0;
        } else {
            // For everything else the standard comparison should be OK
            return QStandardItem::operator<(other);
        }
    }
};

static void setViewDefaults(Utils::TreeView *view)
{
    view->setFrameStyle(QFrame::NoFrame);
    QHeaderView *header = view->header();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setDefaultSectionSize(100);
    header->setMinimumSectionSize(50);
}

static QString displayHeader(MainField header)
{
    switch (header) {
    case MainCallCount:
        return QmlProfilerStatisticsMainView::tr("Calls");
    case MainDetails:
        return QmlProfilerStatisticsMainView::tr("Details");
    case MainLocation:
        return QmlProfilerStatisticsMainView::tr("Location");
    case MainMaxTime:
        return QmlProfilerStatisticsMainView::tr("Longest Time");
    case MainTimePerCall:
        return QmlProfilerStatisticsMainView::tr("Mean Time");
    case MainSelfTime:
        return QmlProfilerStatisticsMainView::tr("Self Time");
    case MainSelfTimeInPercent:
        return QmlProfilerStatisticsMainView::tr("Self Time in Percent");
    case MainMinTime:
        return QmlProfilerStatisticsMainView::tr("Shortest Time");
    case MainTimeInPercent:
        return QmlProfilerStatisticsMainView::tr("Time in Percent");
    case MainTotalTime:
        return QmlProfilerStatisticsMainView::tr("Total Time");
    case MainType:
        return QmlProfilerStatisticsMainView::tr("Type");
    case MainMedianTime:
        return QmlProfilerStatisticsMainView::tr("Median Time");
    case MaxMainField:
        QTC_ASSERT(false, break);
    }
    return QString();
}

static QString displayHeader(RelativeField header, QmlProfilerStatisticsRelation relation)
{
    switch (header) {
    case RelativeLocation:
        return relation == QmlProfilerStatisticsCallees
                ? QmlProfilerStatisticsMainView::tr("Callee")
                : QmlProfilerStatisticsMainView::tr("Caller");
    case RelativeType:
        return displayHeader(MainType);
    case RelativeTotalTime:
        return displayHeader(MainTotalTime);
    case RelativeCallCount:
        return displayHeader(MainCallCount);
    case RelativeDetails:
        return relation == QmlProfilerStatisticsCallees
                ? QmlProfilerStatisticsMainView::tr("Callee Description")
                : QmlProfilerStatisticsMainView::tr("Caller Description");
    case MaxRelativeField:
        QTC_ASSERT(false, break);
    }
    return QString();
}

static void getSourceLocation(QStandardItem *infoItem,
                              std::function<void (const QString &, int, int)> receiver)
{
    int line = infoItem->data(LineRole).toInt();
    int column = infoItem->data(ColumnRole).toInt();
    QString fileName = infoItem->data(FilenameRole).toString();
    if (line != -1 && !fileName.isEmpty())
        receiver(fileName, line, column);
}

QmlProfilerStatisticsView::QmlProfilerStatisticsView(QmlProfilerModelManager *profilerModelManager,
                                                     QWidget *parent)
    : QmlProfilerEventsView(parent)
{
    setObjectName(QLatin1String("QmlProfiler.Statistics.Dock"));
    setWindowTitle(tr("Statistics"));

    QmlProfilerStatisticsModel *model = new QmlProfilerStatisticsModel(profilerModelManager);
    m_mainView.reset(new QmlProfilerStatisticsMainView(model));
    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::gotoSourceLocation,
            this, &QmlProfilerStatisticsView::gotoSourceLocation);
    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::typeSelected,
            this, &QmlProfilerStatisticsView::typeSelected);

    m_calleesView.reset(new QmlProfilerStatisticsRelativesView(
                new QmlProfilerStatisticsRelativesModel(profilerModelManager, model,
                                                        QmlProfilerStatisticsCallees)));
    m_callersView.reset(new QmlProfilerStatisticsRelativesView(
                new QmlProfilerStatisticsRelativesModel(profilerModelManager, model,
                                                        QmlProfilerStatisticsCallers)));
    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::typeSelected,
            m_calleesView.get(), &QmlProfilerStatisticsRelativesView::displayType);
    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::typeSelected,
            m_callersView.get(), &QmlProfilerStatisticsRelativesView::displayType);
    connect(m_calleesView.get(), &QmlProfilerStatisticsRelativesView::typeClicked,
            m_mainView.get(), &QmlProfilerStatisticsMainView::selectType);
    connect(m_callersView.get(), &QmlProfilerStatisticsRelativesView::typeClicked,
            m_mainView.get(), &QmlProfilerStatisticsMainView::selectType);
    connect(m_calleesView.get(), &QmlProfilerStatisticsRelativesView::gotoSourceLocation,
            this, &QmlProfilerStatisticsView::gotoSourceLocation);
    connect(m_callersView.get(), &QmlProfilerStatisticsRelativesView::gotoSourceLocation,
            this, &QmlProfilerStatisticsView::gotoSourceLocation);

    // widget arrangement
    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0,0,0,0);
    groupLayout->setSpacing(0);

    Core::MiniSplitter *splitterVertical = new Core::MiniSplitter;
    splitterVertical->addWidget(m_mainView.get());
    Core::MiniSplitter *splitterHorizontal = new Core::MiniSplitter;
    splitterHorizontal->addWidget(m_callersView.get());
    splitterHorizontal->addWidget(m_calleesView.get());
    splitterHorizontal->setOrientation(Qt::Horizontal);
    splitterVertical->addWidget(splitterHorizontal);
    splitterVertical->setOrientation(Qt::Vertical);
    splitterVertical->setStretchFactor(0,5);
    splitterVertical->setStretchFactor(1,2);
    groupLayout->addWidget(splitterVertical);
    setLayout(groupLayout);
}

void QmlProfilerStatisticsView::clear()
{
    m_mainView->clear();
    m_calleesView->clear();
    m_callersView->clear();
}

QString QmlProfilerStatisticsView::summary(const QVector<int> &typeIds) const
{
    return m_mainView->summary(typeIds);
}

QStringList QmlProfilerStatisticsView::details(int typeId) const
{
    return m_mainView->details(typeId);
}

QModelIndex QmlProfilerStatisticsView::selectedModelIndex() const
{
    return m_mainView->selectedModelIndex();
}

void QmlProfilerStatisticsView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *copyRowAction = 0;
    QAction *copyTableAction = 0;
    QAction *showExtendedStatsAction = 0;
    QAction *getGlobalStatsAction = 0;

    QPoint position = ev->globalPos();

    QList <QAction *> commonActions = QmlProfilerTool::profilerContextMenuActions();
    foreach (QAction *act, commonActions)
        menu.addAction(act);

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
    getGlobalStatsAction = menu.addAction(tr("Show Full Range"));
    if (!m_mainView->isRestrictedToRange())
        getGlobalStatsAction->setEnabled(false);

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == copyRowAction)
            copyRowToClipboard();
        if (selectedAction == copyTableAction)
            copyTableToClipboard();
        if (selectedAction == getGlobalStatsAction)
            emit showFullRange();
        if (selectedAction == showExtendedStatsAction)
            setShowExtendedStatistics(!showExtendedStatistics());
    }
}

bool QmlProfilerStatisticsView::mouseOnTable(const QPoint &position) const
{
    QPoint tableTopLeft = m_mainView->mapToGlobal(QPoint(0,0));
    QPoint tableBottomRight = m_mainView->mapToGlobal(QPoint(m_mainView->width(), m_mainView->height()));
    return (position.x() >= tableTopLeft.x() && position.x() <= tableBottomRight.x() && position.y() >= tableTopLeft.y() && position.y() <= tableBottomRight.y());
}

void QmlProfilerStatisticsView::copyTableToClipboard() const
{
    m_mainView->copyTableToClipboard();
}

void QmlProfilerStatisticsView::copyRowToClipboard() const
{
    m_mainView->copyRowToClipboard();
}

void QmlProfilerStatisticsView::selectByTypeId(int typeIndex)
{
    if (m_mainView->selectedTypeId() != typeIndex)
        m_mainView->selectType(typeIndex);
}

void QmlProfilerStatisticsView::onVisibleFeaturesChanged(quint64 features)
{
    m_mainView->restrictToFeatures(features);
}

void QmlProfilerStatisticsView::setShowExtendedStatistics(bool show)
{
    m_mainView->setShowExtendedStatistics(show);
}

bool QmlProfilerStatisticsView::showExtendedStatistics() const
{
    return m_mainView->showExtendedStatistics();
}

QmlProfilerStatisticsMainView::QmlProfilerStatisticsMainView(QmlProfilerStatisticsModel *model) :
    m_model(model)
{
    setViewDefaults(this);
    setObjectName(QLatin1String("QmlProfilerEventsTable"));

    m_standardItemModel.reset(new QStandardItemModel);
    m_standardItemModel->setSortRole(SortRole);
    setModel(m_standardItemModel.get());
    connect(this, &QAbstractItemView::activated, this, &QmlProfilerStatisticsMainView::jumpToItem);

    connect(m_model.get(), &QmlProfilerStatisticsModel::dataAvailable,
            this, &QmlProfilerStatisticsMainView::buildModel);
    connect(m_model.get(), &QmlProfilerStatisticsModel::notesAvailable,
            this, &QmlProfilerStatisticsMainView::updateNotes);

    setSortingEnabled(true);
    sortByColumn(DEFAULT_SORT_COLUMN, Qt::DescendingOrder);

    buildModel();
}

QmlProfilerStatisticsMainView::~QmlProfilerStatisticsMainView()
{
    clear();
}

void QmlProfilerStatisticsMainView::setHeaderLabels()
{
    for (int i = 0; i < MaxMainField; ++i) {
        m_standardItemModel->setHeaderData(i, Qt::Horizontal,
                                           displayHeader(static_cast<MainField>(i)));
    }
}

void QmlProfilerStatisticsMainView::setShowExtendedStatistics(bool show)
{
    // Not checking if already set because we don't want the first call to skip
    m_showExtendedStatistics = show;
    if (show) {
        showColumn(MainMedianTime);
        showColumn(MainMaxTime);
        showColumn(MainMinTime);
    } else {
        hideColumn(MainMedianTime);
        hideColumn(MainMaxTime);
        hideColumn(MainMinTime);
    }
}

bool QmlProfilerStatisticsMainView::showExtendedStatistics() const
{
    return m_showExtendedStatistics;
}

void QmlProfilerStatisticsMainView::clear()
{
    SortPreserver sorter(this);
    m_standardItemModel->clear();
    m_standardItemModel->setColumnCount(MaxMainField);
    setHeaderLabels();
}

void QmlProfilerStatisticsMainView::buildModel()
{
    clear();

    {
        SortPreserver sorter(this);
        parseModel();
        setShowExtendedStatistics(m_showExtendedStatistics);
        setRootIsDecorated(false);
    }

    resizeColumnToContents(MainLocation);
    resizeColumnToContents(MainType);
}

void QmlProfilerStatisticsMainView::updateNotes(int typeIndex)
{
    const QVector<QmlProfilerStatisticsModel::QmlEventStats> &eventList = m_model->getData();
    const QHash<int, QString> &noteList = m_model->getNotes();
    QStandardItem *rootItem = m_standardItemModel->invisibleRootItem();

    for (int rowIndex = 0; rowIndex < rootItem->rowCount(); ++rowIndex) {
        int rowType = rootItem->child(rowIndex)->data(TypeIdRole).toInt();
        if (rowType == -1 || (rowType != typeIndex && typeIndex != -1))
            continue;
        const QmlProfilerStatisticsModel::QmlEventStats &stats = eventList[rowType];

        for (int columnIndex = 0; columnIndex < rootItem->columnCount(); ++columnIndex) {
            QStandardItem *item = rootItem->child(rowIndex, columnIndex);
            QHash<int, QString>::ConstIterator it = noteList.find(rowType);
            if (it != noteList.end()) {
                item->setBackground(colors()->noteBackground);
                item->setToolTip(it.value());
            } else if (stats.recursive > 0) {
                item->setBackground(colors()->noteBackground);
                item->setToolTip(tr("+%1 in recursive calls")
                                 .arg(Timeline::formatTime(stats.recursive)));
            } else if (!item->toolTip().isEmpty()){
                item->setBackground(colors()->defaultBackground);
                item->setToolTip(QString());
            }
        }
    }
}

void QmlProfilerStatisticsMainView::restrictToFeatures(quint64 features)
{
    m_model->restrictToFeatures(features);
}

bool QmlProfilerStatisticsMainView::isRestrictedToRange() const
{
    return m_model->isRestrictedToRange();
}

QString QmlProfilerStatisticsMainView::summary(const QVector<int> &typeIds) const
{
    return m_model->summary(typeIds);
}

QStringList QmlProfilerStatisticsMainView::details(int typeId) const
{
    return m_model->details(typeId);
}

void QmlProfilerStatisticsMainView::parseModel()
{
    const QVector<QmlProfilerStatisticsModel::QmlEventStats> &eventList = m_model->getData();
    const QVector<QmlEventType> &typeList = m_model->getTypes();

    QmlProfilerStatisticsModel::QmlEventStats rootEventStats;
    rootEventStats.total = rootEventStats.maximum = rootEventStats.minimum = rootEventStats.median
            = m_model->rootDuration();
    rootEventStats.calls = rootEventStats.total > 0 ? 1 : 0;

    for (int typeIndex = -1; typeIndex < eventList.size(); ++typeIndex) {
        const QmlProfilerStatisticsModel::QmlEventStats &stats = typeIndex >= 0
                ? eventList[typeIndex] : rootEventStats;
        if (stats.calls == 0)
            continue;

        const QmlEventType &type = typeIndex >= 0 ? typeList[typeIndex] : *rootEventType();
        QStandardItem *rootItem = m_standardItemModel->invisibleRootItem();
        QList<QStandardItem *> newRow;

        newRow << new StatisticsViewItem(
                      type.displayName().isEmpty() ? tr("<bytecode>") : type.displayName(),
                      type.displayName());

        QString typeString = QmlProfilerStatisticsModel::nameForType(type.rangeType());
        newRow << new StatisticsViewItem(typeString, typeString);

        const double percent = m_model->durationPercent(typeIndex);
        newRow << new StatisticsViewItem(QString::number(percent, 'f', 2)
                                         + QLatin1String(" %"), percent);

        newRow << new StatisticsViewItem(
                      Timeline::formatTime(stats.totalNonRecursive()),
                      stats.totalNonRecursive());

        const double percentSelf = m_model->durationSelfPercent(typeIndex);
        newRow << new StatisticsViewItem(QString::number(percentSelf, 'f', 2)
                                         + QLatin1String(" %"), percentSelf);

        newRow << new StatisticsViewItem(Timeline::formatTime(stats.self),
                                         stats.self);

        newRow << new StatisticsViewItem(QString::number(stats.calls), stats.calls);

        const qint64 timePerCall = stats.average();
        newRow << new StatisticsViewItem(Timeline::formatTime(timePerCall),
                                         timePerCall);

        newRow << new StatisticsViewItem(Timeline::formatTime(stats.median),
                                         stats.median);

        newRow << new StatisticsViewItem(Timeline::formatTime(stats.maximum),
                                         stats.maximum);

        newRow << new StatisticsViewItem(Timeline::formatTime(stats.minimum),
                                         stats.minimum);

        newRow << new StatisticsViewItem(type.data().isEmpty() ? tr("Source code not available")
                                                               : type.data(), type.data());

        // no edit
        foreach (QStandardItem *item, newRow)
            item->setEditable(false);

        // metadata
        QStandardItem *first = newRow.at(MainLocation);
        first->setData(typeIndex, TypeIdRole);
        const QmlEventLocation location(type.location());
        first->setData(location.filename(), FilenameRole);
        first->setData(location.line(), LineRole);
        first->setData(location.column(), ColumnRole);

        // append
        rootItem->appendRow(newRow);
    }
}

QStandardItem *QmlProfilerStatisticsMainView::itemFromIndex(const QModelIndex &index) const
{
    QStandardItem *indexItem = m_standardItemModel->itemFromIndex(index);
    if (indexItem->parent())
        return indexItem->parent()->child(indexItem->row());
    else
        return m_standardItemModel->item(index.row());
}

int QmlProfilerStatisticsMainView::selectedTypeId() const
{
    QModelIndex index = selectedModelIndex();
    if (!index.isValid())
        return -1;
    QStandardItem *item = m_standardItemModel->item(index.row());
    return item->data(TypeIdRole).toInt();
}

void QmlProfilerStatisticsMainView::jumpToItem(const QModelIndex &index)
{
    QStandardItem *infoItem = itemFromIndex(index);

    // show in editor
    getSourceLocation(infoItem, [this](const QString &fileName, int line, int column) {
        emit gotoSourceLocation(fileName, line, column);
    });

    // show in callers/callees subwindow
    emit typeSelected(infoItem->data(TypeIdRole).toInt());
}

void QmlProfilerStatisticsMainView::selectItem(const QStandardItem *item)
{
    // If the same item is already selected, don't reselect it.
    QModelIndex index = m_standardItemModel->indexFromItem(item);
    if (index != currentIndex()) {
        setCurrentIndex(index);

        // show in callers/callees subwindow
        emit typeSelected(itemFromIndex(index)->data(TypeIdRole).toInt());
    }
}

void QmlProfilerStatisticsMainView::selectType(int typeIndex)
{
    for (int i = 0; i < m_standardItemModel->rowCount(); i++) {
        QStandardItem *infoItem = m_standardItemModel->item(i);
        if (infoItem->data(TypeIdRole).toInt() == typeIndex) {
            selectItem(infoItem);
            return;
        }
    }
}

QModelIndex QmlProfilerStatisticsMainView::selectedModelIndex() const
{
    QModelIndexList sel = selectedIndexes();
    if (sel.isEmpty())
        return QModelIndex();
    else
        return sel.first();
}

QString QmlProfilerStatisticsMainView::textForItem(QStandardItem *item) const
{
    QString str;

    // item's data
    int colCount = m_standardItemModel->columnCount();
    for (int j = 0; j < colCount; ++j) {
        QStandardItem *colItem = item->parent() ? item->parent()->child(item->row(),j) :
                                                  m_standardItemModel->item(item->row(),j);
        str += colItem->data(Qt::DisplayRole).toString();
        if (j < colCount-1) str += QLatin1Char('\t');
    }
    str += QLatin1Char('\n');

    return str;
}

void QmlProfilerStatisticsMainView::copyTableToClipboard() const
{
    QString str;
    // headers
    int columnCount = m_standardItemModel->columnCount();
    for (int i = 0; i < columnCount; ++i) {
        str += m_standardItemModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        if (i < columnCount - 1)
            str += QLatin1Char('\t');
        else
            str += QLatin1Char('\n');
    }
    // data
    int rowCount = m_standardItemModel->rowCount();
    for (int i = 0; i != rowCount; ++i) {
        str += textForItem(m_standardItemModel->item(i));
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

void QmlProfilerStatisticsMainView::copyRowToClipboard() const
{
    QString str = textForItem(m_standardItemModel->itemFromIndex(selectedModelIndex()));
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

QmlProfilerStatisticsRelativesView::QmlProfilerStatisticsRelativesView(
        QmlProfilerStatisticsRelativesModel *model) :
    m_model(model)
{
    setViewDefaults(this);
    QStandardItemModel *itemModel = new QStandardItemModel(this);
    itemModel->setSortRole(SortRole);
    setModel(itemModel);
    setRootIsDecorated(false);
    updateHeader();

    setSortingEnabled(true);
    sortByColumn(DEFAULT_SORT_COLUMN, Qt::DescendingOrder);

    connect(this, &QAbstractItemView::activated,
            this, &QmlProfilerStatisticsRelativesView::jumpToItem);

    // Clear when new data available as the selection may be invalid now.
    connect(m_model.get(), &QmlProfilerStatisticsRelativesModel::dataAvailable,
            this, &QmlProfilerStatisticsRelativesView::clear);
}

QmlProfilerStatisticsRelativesView::~QmlProfilerStatisticsRelativesView()
{
}

void QmlProfilerStatisticsRelativesView::displayType(int typeIndex)
{
    SortPreserver sorter(this);
    rebuildTree(m_model->getData(typeIndex));

    updateHeader();
    resizeColumnToContents(RelativeLocation);
}

void QmlProfilerStatisticsRelativesView::rebuildTree(
        const QVector<QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData> &data)
{
    Q_ASSERT(treeModel());
    treeModel()->clear();

    QStandardItem *topLevelItem = treeModel()->invisibleRootItem();
    const QVector<QmlEventType> &typeList = m_model->getTypes();

    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData &stats = *it;
        const QmlEventType &type = stats.typeIndex != -1 ? typeList[stats.typeIndex]
                                                         : *rootEventType();
        QList<QStandardItem *> newRow;

        // ToDo: here we were going to search for the data in the other model
        // maybe we should store the data in this model and get it here
        // no indirections at this level of abstraction!
        newRow << new StatisticsViewItem(
                      type.displayName().isEmpty() ? tr("<bytecode>") : type.displayName(),
                      type.displayName());
        const QString typeName = QmlProfilerStatisticsModel::nameForType(type.rangeType());
        newRow << new StatisticsViewItem(typeName, typeName);
        newRow << new StatisticsViewItem(Timeline::formatTime(stats.duration),
                                         stats.duration);
        newRow << new StatisticsViewItem(QString::number(stats.calls), stats.calls);
        newRow << new StatisticsViewItem(type.data().isEmpty() ? tr("Source code not available") :
                                                                 type.data(), type.data());

        QStandardItem *first = newRow.at(RelativeLocation);
        first->setData(stats.typeIndex, TypeIdRole);
        const QmlEventLocation location(type.location());
        first->setData(location.filename(), FilenameRole);
        first->setData(location.line(), LineRole);
        first->setData(location.column(), ColumnRole);

        if (stats.isRecursive) {
            foreach (QStandardItem *item, newRow) {
                item->setBackground(colors()->noteBackground);
                item->setToolTip(tr("called recursively"));
            }
        }

        foreach (QStandardItem *item, newRow)
            item->setEditable(false);

        topLevelItem->appendRow(newRow);
    }
}

void QmlProfilerStatisticsRelativesView::clear()
{
    if (treeModel()) {
        SortPreserver sorter(this);
        treeModel()->clear();
        updateHeader();
    }
}

void QmlProfilerStatisticsRelativesView::updateHeader()
{
    const QmlProfilerStatisticsRelation relation = m_model->relation();
    if (QStandardItemModel *model = treeModel()) {
        model->setColumnCount(MaxRelativeField);
        for (int i = 0; i < MaxRelativeField; ++i) {
            model->setHeaderData(i, Qt::Horizontal,
                                 displayHeader(static_cast<RelativeField>(i), relation));
        }
    }
}

QStandardItemModel *QmlProfilerStatisticsRelativesView::treeModel()
{
    return qobject_cast<QStandardItemModel *>(model());
}

void QmlProfilerStatisticsRelativesView::jumpToItem(const QModelIndex &index)
{
    if (treeModel()) {
        QStandardItem *infoItem = treeModel()->item(index.row());
        // show in editor
        getSourceLocation(infoItem, [this](const QString &fileName, int line, int column) {
            emit gotoSourceLocation(fileName, line, column);
        });

        emit typeClicked(infoItem->data(TypeIdRole).toInt());
    }
}

} // namespace Internal
} // namespace QmlProfiler
