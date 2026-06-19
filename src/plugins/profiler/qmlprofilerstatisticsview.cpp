// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertr.h"
#include "qmlprofilerstatisticsview.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertool.h"

#include <qmldebug/qmleventlocation.h>

#include <texteditor/textmark.h>

#include <coreplugin/minisplitter.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <tracing/timelineformattime.h>

#include <QApplication>
#include <QClipboard>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMultiHash>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include <functional>

using namespace Utils;

namespace Profiler::Internal {

class QmlProfilerTextMark : public TextEditor::TextMark
{
public:
    QmlProfilerTextMark(QmlProfilerStatisticsView *statisticsView,
                        int typeId, const FilePath &fileName, int lineNumber);

    void addTypeId(int typeId);

    bool addToolTipContent(QLayout *target) const override;

private:
    QmlProfilerStatisticsView *m_statisticsView;
    QList<int> m_typeIds;
};

class QmlProfilerTextMarkModel : public QObject
{
public:
    QmlProfilerTextMarkModel(QmlProfilerStatisticsView *statisticsView, QObject *parent = nullptr);
    ~QmlProfilerTextMarkModel() override;

    void clear();
    void addTextMarkId(int typeId, const QmlDebug::QmlEventLocation &location);
    void createMarks(const QString &fileName);

    void showTextMarks();
    void hideTextMarks();

private:
    struct TextMarkId {
        int typeId;
        int lineNumber;
        int columnNumber;
    };

    QmlProfilerStatisticsView *m_statisticsView = nullptr;
    QMultiHash<QString, TextMarkId> m_ids;
    QList<QmlProfilerTextMark *> m_marks;
};

const int DEFAULT_SORT_COLUMN = MainTimeInPercent;

static void setViewDefaults(Utils::TreeView *view)
{
    view->setFrameStyle(QFrame::NoFrame);
    QHeaderView *header = view->header();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setDefaultSectionSize(100);
    header->setMinimumSectionSize(50);
}

static void getSourceLocation(const QModelIndex &index,
                              std::function<void (const QString &, int, int)> receiver)
{
    const int line = index.data(LineRole).toInt();
    const int column = index.data(ColumnRole).toInt();
    const QString fileName = index.data(FilenameRole).toString();
    if (line != -1 && !fileName.isEmpty())
        receiver(fileName, line, column);
}

// QmlProfilerStatisticsView

QmlProfilerStatisticsView::QmlProfilerStatisticsView(QmlProfilerModelManager *profilerModelManager,
                                                     QWidget *parent)
    : QmlProfilerEventsView(parent)
{
    setObjectName(QLatin1String("QmlProfiler.Statistics.Dock"));
    setWindowTitle(Tr::tr("Statistics"));

    m_textMarkModel = new QmlProfilerTextMarkModel(this, this);
    connect(profilerModelManager, &QmlProfilerModelManager::initialized,
            m_textMarkModel, &QmlProfilerTextMarkModel::hideTextMarks);
    connect(profilerModelManager, &QmlProfilerModelManager::typesCleared,
            m_textMarkModel, &QmlProfilerTextMarkModel::clear);
    connect(profilerModelManager, &QmlProfilerModelManager::typeLocationAdded,
            m_textMarkModel, &QmlProfilerTextMarkModel::addTextMarkId);
    connect(profilerModelManager, &Timeline::TimelineTraceManager::loadFinished,
            m_textMarkModel, &QmlProfilerTextMarkModel::showTextMarks);

    auto model = new QmlProfilerStatisticsModel(profilerModelManager);
    m_mainView.reset(new QmlProfilerStatisticsMainView(model));
    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::gotoSourceLocation,
            this, &QmlProfilerStatisticsView::gotoSourceLocation);

    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::typeClicked,
            this, [this, profilerModelManager](int typeIndex) {
        // Statistics view has an extra type for "whole program". Translate that into "invalid" for
        // others.
        emit typeSelected((typeIndex < profilerModelManager->numEventTypes())
                          ? typeIndex : QmlProfilerStatisticsModel::s_invalidTypeId);
    });

    m_calleesView.reset(new QmlProfilerStatisticsRelativesView(
                new QmlProfilerStatisticsRelativesModel(profilerModelManager, model,
                                                        QmlProfilerStatisticsCallees)));
    m_callersView.reset(new QmlProfilerStatisticsRelativesView(
                new QmlProfilerStatisticsRelativesModel(profilerModelManager, model,
                                                        QmlProfilerStatisticsCallers)));
    connect(m_calleesView.get(), &QmlProfilerStatisticsRelativesView::typeClicked,
            m_mainView.get(), &QmlProfilerStatisticsMainView::jumpToItem);
    connect(m_callersView.get(), &QmlProfilerStatisticsRelativesView::typeClicked,
            m_mainView.get(), &QmlProfilerStatisticsMainView::jumpToItem);
    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::propagateTypeIndex,
            m_calleesView.get(), &QmlProfilerStatisticsRelativesView::displayType);
    connect(m_mainView.get(), &QmlProfilerStatisticsMainView::propagateTypeIndex,
            m_callersView.get(), &QmlProfilerStatisticsRelativesView::displayType);

    // widget arrangement
    auto groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0,0,0,0);
    groupLayout->setSpacing(0);

    auto splitterVertical = new Core::MiniSplitter;
    splitterVertical->addWidget(m_mainView.get());
    auto splitterHorizontal = new Core::MiniSplitter;
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

QString QmlProfilerStatisticsView::summary(const QList<int> &typeIds) const
{
    return m_mainView->summary(typeIds);
}

QStringList QmlProfilerStatisticsView::details(int typeId) const
{
    return m_mainView->details(typeId);
}

void QmlProfilerStatisticsView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *copyRowAction = nullptr;
    QAction *copyTableAction = nullptr;
    QAction *showExtendedStatsAction = nullptr;
    QAction *getGlobalStatsAction = nullptr;

    QPoint position = ev->globalPos();

    const QList <QAction *> commonActions = QmlProfilerTool::profilerContextMenuActions();
    for (QAction *act : commonActions)
        menu.addAction(act);

    if (mouseOnTable(position)) {
        menu.addSeparator();
        if (m_mainView->selectedModelIndex().isValid())
            copyRowAction = menu.addAction(Tr::tr("Copy Row"));
        copyTableAction = menu.addAction(Tr::tr("Copy Table"));

        showExtendedStatsAction = menu.addAction(Tr::tr("Extended Event Statistics"));
        showExtendedStatsAction->setCheckable(true);
        showExtendedStatsAction->setChecked(m_mainView->showExtendedStatistics());
    }

    menu.addSeparator();
    getGlobalStatsAction = menu.addAction(Tr::tr("Show Full Range"));
    if (!m_mainView->isRestrictedToRange())
        getGlobalStatsAction->setEnabled(false);

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == copyRowAction)
            m_mainView->copyRowToClipboard();
        if (selectedAction == copyTableAction)
            m_mainView->copyTableToClipboard();
        if (selectedAction == getGlobalStatsAction)
            emit showFullRange();
        if (selectedAction == showExtendedStatsAction)
            m_mainView->setShowExtendedStatistics(showExtendedStatsAction->isChecked());
    }
}

bool QmlProfilerStatisticsView::mouseOnTable(const QPoint &position) const
{
    QPoint tableTopLeft = m_mainView->mapToGlobal(QPoint(0,0));
    QPoint tableBottomRight = m_mainView->mapToGlobal(QPoint(m_mainView->width(),
                                                             m_mainView->height()));
    return position.x() >= tableTopLeft.x() && position.x() <= tableBottomRight.x()
            && position.y() >= tableTopLeft.y() && position.y() <= tableBottomRight.y();
}

void QmlProfilerStatisticsView::selectByTypeId(int typeIndex)
{
    // Other models cannot discern between "nothing" and "Main Program". So don't propagate invalid
    // typeId, if we already have whole program selected.
    if (typeIndex >= 0
            || m_mainView->currentIndex().data(TypeIdRole).toInt()
            != QmlProfilerStatisticsModel::s_mainEntryTypeId) {
        m_mainView->displayTypeIndex(typeIndex);
    }
}

void QmlProfilerStatisticsView::onVisibleFeaturesChanged(quint64 features)
{
    m_mainView->restrictToFeatures(features);
}

void QmlProfilerStatisticsView::createMarks(const QString &fileName)
{
    m_textMarkModel->createMarks(fileName);
}

//  QmlProfilerStatisticsMainView

QmlProfilerStatisticsMainView::QmlProfilerStatisticsMainView(QmlProfilerStatisticsModel *model) :
    m_model(model)
{
    setViewDefaults(this);
    setObjectName(QLatin1String("QmlProfilerEventsTable"));

    auto sortModel = new QSortFilterProxyModel(this);
    sortModel->setSourceModel(model);
    sortModel->setSortRole(SortRole);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setFilterRole(FilterRole);
    sortModel->setFilterKeyColumn(MainType);
    sortModel->setFilterFixedString("+");

    setModel(sortModel);

    connect(this, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
        jumpToItem(index.data(TypeIdRole).toInt());
    });

    setSortingEnabled(true);
    sortByColumn(DEFAULT_SORT_COLUMN, Qt::DescendingOrder);

    setShowExtendedStatistics(m_showExtendedStatistics);
    setRootIsDecorated(false);

    resizeColumnToContents(MainLocation);
    resizeColumnToContents(MainType);
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

void QmlProfilerStatisticsMainView::restrictToFeatures(quint64 features)
{
    m_model->restrictToFeatures(features);
}

bool QmlProfilerStatisticsMainView::isRestrictedToRange() const
{
    return m_model->isRestrictedToRange();
}

QString QmlProfilerStatisticsMainView::summary(const QList<int> &typeIds) const
{
    return m_model->summary(typeIds);
}

QStringList QmlProfilerStatisticsMainView::details(int typeId) const
{
    return m_model->details(typeId);
}

void QmlProfilerStatisticsMainView::jumpToItem(int typeIndex)
{
    displayTypeIndex(typeIndex);

    auto sortModel = qobject_cast<const QSortFilterProxyModel *>(model());
    QTC_ASSERT(sortModel, return);

    QAbstractItemModel *sourceModel = sortModel->sourceModel();
    QTC_ASSERT(sourceModel, return);

    // show in editor
    getSourceLocation(sourceModel->index(typeIndex, MainLocation),
                      [this](const QString &fileName, int line, int column) {
        emit gotoSourceLocation(fileName, line, column);
    });

    emit typeClicked(typeIndex);
}

void QmlProfilerStatisticsMainView::displayTypeIndex(int typeIndex)
{
    if (typeIndex < 0) {
        setCurrentIndex(QModelIndex());
    } else {
        auto sortModel = qobject_cast<const QSortFilterProxyModel *>(model());
        QTC_ASSERT(sortModel, return);

        QAbstractItemModel *sourceModel = sortModel->sourceModel();
        QTC_ASSERT(sourceModel, return);

        if (typeIndex < sourceModel->rowCount()) {
            QModelIndex sourceIndex = sourceModel->index(typeIndex, MainCallCount);
            QTC_ASSERT(sourceIndex.data(TypeIdRole).toInt() == typeIndex, return);
            setCurrentIndex(sourceIndex.data(SortRole).toInt() > 0
                            ? sortModel->mapFromSource(sourceIndex)
                            : QModelIndex());
        } else {
            setCurrentIndex(QModelIndex());
        }
    }

    // show in callers/callees subwindow
    emit propagateTypeIndex(typeIndex);
}

QModelIndex QmlProfilerStatisticsMainView::selectedModelIndex() const
{
    QModelIndexList sel = selectedIndexes();
    if (sel.isEmpty())
        return QModelIndex();
    else
        return sel.first();
}

QString QmlProfilerStatisticsMainView::textForItem(const QModelIndex &index) const
{
    QString str;

    // item's data
    const int colCount = model()->columnCount();
    for (int j = 0; j < colCount; ++j) {
        const QModelIndex cellIndex = model()->index(index.row(), j);
        str += cellIndex.data(Qt::DisplayRole).toString();
        if (j < colCount-1) str += QLatin1Char('\t');
    }
    str += QLatin1Char('\n');

    return str;
}

void QmlProfilerStatisticsMainView::copyTableToClipboard() const
{
    QString str;

    const QAbstractItemModel *itemModel = model();

    // headers
    const int columnCount = itemModel->columnCount();
    for (int i = 0; i < columnCount; ++i) {
        str += itemModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        if (i < columnCount - 1)
            str += QLatin1Char('\t');
        else
            str += QLatin1Char('\n');
    }

    // data
    const int rowCount = itemModel->rowCount();
    for (int i = 0; i != rowCount; ++i)
        str += textForItem(itemModel->index(i, 0));

    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard->supportsSelection())
        clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

void QmlProfilerStatisticsMainView::copyRowToClipboard() const
{
    QString str = textForItem(selectedModelIndex());
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard->supportsSelection())
        clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

//  QmlProfilerStatisticsRelativesView

QmlProfilerStatisticsRelativesView::QmlProfilerStatisticsRelativesView(
        QmlProfilerStatisticsRelativesModel *model) :
    m_model(model)
{
    setViewDefaults(this);
    auto sortModel = new QSortFilterProxyModel(this);
    sortModel->setSourceModel(model);
    sortModel->setSortRole(SortRole);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    setModel(sortModel);
    setRootIsDecorated(false);

    setSortingEnabled(true);
    sortByColumn(DEFAULT_SORT_COLUMN, Qt::DescendingOrder);

    connect(this, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
        jumpToItem(index.data(TypeIdRole).toInt());
    });
}

void QmlProfilerStatisticsRelativesView::displayType(int typeIndex)
{
    model()->setData(QModelIndex(), typeIndex, TypeIdRole);
    resizeColumnToContents(RelativeLocation);
}

void QmlProfilerStatisticsRelativesView::jumpToItem(int typeIndex)
{
    emit typeClicked(typeIndex);
}

QmlProfilerTextMark::QmlProfilerTextMark(QmlProfilerStatisticsView *statisticsView,
                                         int typeId,
                                         const FilePath &fileName, int lineNumber)
    : TextMark(fileName, lineNumber, {Tr::tr("QML Profiler"), Constants::TEXT_MARK_CATEGORY})
    , m_statisticsView(statisticsView)
{
    addTypeId(typeId);
}

void QmlProfilerTextMark::addTypeId(int typeId)
{
    m_typeIds.append(typeId);
    QTC_ASSERT(m_statisticsView, return);
    setLineAnnotation(m_statisticsView->summary(m_typeIds));
}

bool QmlProfilerTextMark::addToolTipContent(QLayout *target) const
{
    QTC_ASSERT(m_statisticsView, return false);

    auto layout = new QGridLayout;
    layout->setHorizontalSpacing(10);
    for (int row = 0, rowEnd = m_typeIds.length(); row != rowEnd; ++row) {
        int typeId = m_typeIds[row];
        const QStringList typeDetails = m_statisticsView->details(m_typeIds[row]);
        for (int column = 0, columnEnd = typeDetails.length(); column != columnEnd; ++column) {
            QLabel *label = new QLabel;
            label->setAlignment(column == columnEnd - 1 ? Qt::AlignRight : Qt::AlignLeft);
            if (column == 0) {
                label->setTextFormat(Qt::RichText);
                label->setTextInteractionFlags(Qt::LinksAccessibleByMouse
                                               | Qt::LinksAccessibleByKeyboard);
                label->setText(QString("<a href='selectType' style='text-decoration:none'>%1</a>")
                                   .arg(typeDetails[column]));
                QObject::connect(label, &QLabel::linkActivated, m_statisticsView,
                                 [this, typeId]() { emit m_statisticsView->typeSelected(typeId); });
            } else {
                label->setTextFormat(Qt::PlainText);
                label->setText(typeDetails[column]);
            }
            layout->addWidget(label, row, column);
        }
    }

    target->addItem(layout);
    return true;
}

QmlProfilerTextMarkModel::QmlProfilerTextMarkModel(QmlProfilerStatisticsView *statisticsView,
                                                   QObject *parent)
    : QObject(parent)
    , m_statisticsView(statisticsView)
{}

QmlProfilerTextMarkModel::~QmlProfilerTextMarkModel()
{
    qDeleteAll(m_marks);
}

void QmlProfilerTextMarkModel::clear()
{
    qDeleteAll(m_marks);
    m_marks.clear();
    m_ids.clear();
}

void QmlProfilerTextMarkModel::addTextMarkId(int typeId,
                                             const QmlDebug::QmlEventLocation &location)
{
    m_ids.insert(location.filename(), {typeId, location.line(), location.column()});
}

void QmlProfilerTextMarkModel::createMarks(const QString &fileName)
{
    auto first = m_ids.find(fileName);
    QVarLengthArray<TextMarkId> ids;

    for (auto it = first; it != m_ids.end() && it.key() == fileName;) {
        ids.append({it->typeId, it->lineNumber > 0 ? it->lineNumber : 1, it->columnNumber});
        it = m_ids.erase(it);
    }

    std::sort(ids.begin(), ids.end(), [](const TextMarkId &a, const TextMarkId &b) {
        return (a.lineNumber == b.lineNumber) ? (a.columnNumber < b.columnNumber)
                                              : (a.lineNumber < b.lineNumber);
    });

    int lineNumber = -1;
    for (const auto &id : ids) {
        if (id.lineNumber == lineNumber) {
            m_marks.last()->addTypeId(id.typeId);
        } else {
            lineNumber = id.lineNumber;
            m_marks << new QmlProfilerTextMark(m_statisticsView,
                                               id.typeId,
                                               FilePath::fromString(fileName),
                                               id.lineNumber);
        }
    }
}

void QmlProfilerTextMarkModel::showTextMarks()
{
    for (QmlProfilerTextMark *mark : std::as_const(m_marks))
        mark->setVisible(true);
}

void QmlProfilerTextMarkModel::hideTextMarks()
{
    for (QmlProfilerTextMark *mark : std::as_const(m_marks))
        mark->setVisible(false);
}

} // namespace Profiler::Internal
