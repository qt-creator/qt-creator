// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerfindingsview.h"

#include "profilertr.h"
#include "qmlprofilertool.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QContextMenuEvent>
#include <QFile>
#include <QHeaderView>
#include <QJsonDocument>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

namespace Profiler::Internal {

static void setViewDefaults(Utils::TreeView *view)
{
    view->setFrameStyle(QFrame::NoFrame);
    QHeaderView *header = view->header();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setDefaultSectionSize(100);
    header->setMinimumSectionSize(50);
}

QmlProfilerFindingsView::QmlProfilerFindingsView(QmlProfilerModelManager *profilerModelManager,
                                                 QWidget *parent)
    : QmlProfilerEventsView(parent)
    , m_modelManager(profilerModelManager)
{
    setObjectName("QmlProfiler.Findings.Dock");
    setWindowTitle(Tr::tr("Findings"));

    m_mainView.reset(new QmlProfilerFindingsMainView(
        new QmlProfilerFindingsModel(profilerModelManager)));
    connect(m_mainView.get(), &QmlProfilerFindingsMainView::gotoSourceLocation,
            this, &QmlProfilerFindingsView::gotoSourceLocation);
    connect(m_mainView.get(), &QmlProfilerFindingsMainView::typeClicked,
            this, &QmlProfilerFindingsView::typeSelected);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_mainView.get());
    setLayout(layout);
}

QmlProfilerFindingsView::~QmlProfilerFindingsView() = default;

void QmlProfilerFindingsView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    const QList<QAction *> commonActions = QmlProfilerTool::profilerContextMenuActions();
    for (QAction *action : commonActions)
        menu.addAction(action);

    menu.addSeparator();
    QAction *exportAction = menu.addAction(Tr::tr("Export Findings..."));
    exportAction->setEnabled(!m_mainView->findings().isEmpty());

    if (menu.exec(ev->globalPos()) == exportAction)
        exportFindings();
}

void QmlProfilerFindingsView::exportFindings() const
{
    const Utils::FilePath filePath = Utils::FileUtils::getSaveFilePath(
        Tr::tr("Export Findings"), {}, Tr::tr("JSON Files (*.json);;All Files (*)"));
    if (filePath.isEmpty())
        return;

    const QJsonObject json = findingsToJson(m_mainView->findings(),
                                            m_modelManager->traceStart(),
                                            m_modelManager->traceEnd());
    const Utils::Result<qint64> result
        = filePath.writeFileContents(QJsonDocument(json).toJson(QJsonDocument::Indented));
    if (!result)
        QmlProfilerTool::showNonmodalWarning(result.error());
}

void QmlProfilerFindingsView::selectByTypeId(int typeIndex)
{
    m_mainView->selectByTypeId(typeIndex);
}

void QmlProfilerFindingsView::onVisibleFeaturesChanged(quint64 features)
{
    // Findings are not filtered by feature: a single finding can be derived from several
    // features, and dropping it because one of them was hidden would misrepresent the
    // trace. Restricting the trace to a range does recompute them, through the loader the
    // model registers with the manager.
    Q_UNUSED(features)
}

QmlProfilerFindingsMainView::QmlProfilerFindingsMainView(QmlProfilerFindingsModel *model)
    : m_model(model)
{
    setViewDefaults(this);
    setObjectName("QmlProfilerFindingsTable");

    auto sortModel = new QSortFilterProxyModel(this);
    sortModel->setSourceModel(model);
    sortModel->setSortRole(QmlProfilerFindingsModel::SortRole);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    setModel(sortModel);

    connect(this, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
        // Only findings that carry a line can be shown in an editor. A failed image load is
        // located by its URL and has none; the trace reports line 0 for those, and for
        // Compiling ranges, so anything below 1 must not be turned into a jump.
        const int line = index.data(QmlProfilerFindingsModel::LineRole).toInt();
        const QString fileName = index.data(QmlProfilerFindingsModel::FilenameRole).toString();
        if (line > 0 && !fileName.isEmpty()) {
            emit gotoSourceLocation(fileName, line,
                                    index.data(QmlProfilerFindingsModel::ColumnRole).toInt());
        }

        const int typeIndex = index.data(QmlProfilerFindingsModel::TypeIdRole).toInt();
        if (typeIndex != -1)
            emit typeClicked(typeIndex);
    });

    setSortingEnabled(true);
    sortByColumn(QmlProfilerFindingsModel::ColumnSeverity, Qt::DescendingOrder);
    setRootIsDecorated(false);

    resizeColumnToContents(QmlProfilerFindingsModel::ColumnSeverity);
    resizeColumnToContents(QmlProfilerFindingsModel::ColumnFinding);
}

void QmlProfilerFindingsMainView::selectByTypeId(int typeIndex)
{
    if (typeIndex == -1) {
        setCurrentIndex(QModelIndex());
        return;
    }

    auto sortModel = qobject_cast<const QSortFilterProxyModel *>(model());
    QTC_ASSERT(sortModel, return);

    for (int row = 0, rowCount = sortModel->rowCount(); row < rowCount; ++row) {
        const QModelIndex index = sortModel->index(row, QmlProfilerFindingsModel::ColumnFinding);
        if (index.data(QmlProfilerFindingsModel::TypeIdRole).toInt() == typeIndex) {
            setCurrentIndex(index);
            scrollTo(index);
            return;
        }
    }
}

} // namespace Profiler::Internal
