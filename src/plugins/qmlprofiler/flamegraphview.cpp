// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only With Qt-GPL-exception-1.0

#include "flamegraphview.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertool.h"
#include "qmlprofilertr.h"

#include <tracing/flamegraphwidget.h>

#include <QContextMenuEvent>
#include <QMenu>
#include <QVBoxLayout>

namespace QmlProfiler::Internal {

FlameGraphView::FlameGraphView(QmlProfilerModelManager *manager, QWidget *parent)
    : QmlProfilerEventsView(parent)
    , m_model(new FlameGraphModel(manager, this))
{
    setObjectName("QmlProfiler.FlameGraph.Dock");
    setWindowTitle(Tr::tr("Flame Graph"));

    m_content = new Timeline::FlameGraphWidget(m_model, this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_content);

    connect(m_content, &Timeline::FlameGraphWidget::typeSelected,
            this, &FlameGraphView::typeSelected);
    connect(m_content, &Timeline::FlameGraphWidget::gotoSourceLocation,
            this, &FlameGraphView::gotoSourceLocation);

    using Role = FlameGraphModel;
    m_content->setTypeIdRole(Role::TypeIdRole);
    m_content->setSourceRoles(Role::FilenameRole, Role::LineRole, Role::ColumnRole);
    m_content->setSummaryRole(Role::DetailsRole);
    m_content->setDetailsTitleRole(Role::TypeRole);
    m_content->setNoteRole(Role::NoteRole);
    m_content->setSizeRoles({
        {Role::DurationRole,    Tr::tr("Total Time")},
        {Role::MemoryRole,      Tr::tr("Memory")},
        {Role::AllocationsRole, Tr::tr("Allocations")},
    });
    m_content->setDetailsRoles({
        {Role::DetailsRole,     Tr::tr("Details")},
        {Role::CallCountRole,   Tr::tr("Calls")},
        {Role::DurationRole,    Tr::tr("Total Time")},
        {Role::TimePerCallRole, Tr::tr("Mean Time")},
        {Role::LocationRole,    Tr::tr("Location")},
        {Role::MemoryRole,      Tr::tr("Memory")},
        {Role::AllocationsRole, Tr::tr("Allocations")},
    });
    m_content->setOthersText(Tr::tr("Various Events"));
}

void FlameGraphView::selectByTypeId(int typeIndex)
{
    m_content->selectByTypeId(typeIndex);
}

void FlameGraphView::onVisibleFeaturesChanged(quint64 features)
{
    m_model->restrictToFeatures(features);
}

void FlameGraphView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;

    QPoint position = ev->globalPos();

    menu.addActions(QmlProfilerTool::profilerContextMenuActions());
    menu.addSeparator();
    QAction *getGlobalStatsAction = menu.addAction(Tr::tr("Show Full Range"));
    getGlobalStatsAction->setEnabled(m_model->modelManager()->isRestrictedToRange());
    QAction *resetAction = menu.addAction(Tr::tr("Reset Flame Graph"));
    resetAction->setEnabled(m_content->isZoomed());

    const QAction *selected = menu.exec(position);
    if (selected == getGlobalStatsAction)
        emit showFullRange();
    else if (selected == resetAction)
        m_content->resetRoot();
}

} // namespace QmlProfiler::Internal
