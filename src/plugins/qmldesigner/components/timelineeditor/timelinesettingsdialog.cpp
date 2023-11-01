// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinesettingsdialog.h"

#include "timelineanimationform.h"
#include "timelineform.h"
#include "timelineicons.h"
#include "timelinesettingsmodel.h"
#include "timelineview.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <exception>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <variantproperty.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableView>
#include <QToolBar>

namespace QmlDesigner {

static void deleteAllTabs(QTabWidget *tabWidget)
{
    while (tabWidget->count() > 0) {
        QWidget *w = tabWidget->widget(0);
        tabWidget->removeTab(0);
        delete w;
    }
}

static ModelNode getAnimationFromTabWidget(QTabWidget *tabWidget)
{
    QWidget *w = tabWidget->currentWidget();
    if (w)
        return qobject_cast<TimelineAnimationForm *>(w)->animation();
    return ModelNode();
}

static QmlTimeline getTimelineFromTabWidget(QTabWidget *tabWidget)
{
    QWidget *w = tabWidget->currentWidget();
    if (w)
        return qobject_cast<TimelineForm *>(w)->timeline();
    return QmlTimeline();
}

static void setTabForTimeline(QTabWidget *tabWidget, const QmlTimeline &timeline)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        QWidget *w = tabWidget->widget(i);
        if (qobject_cast<TimelineForm *>(w)->timeline() == timeline) {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }
}

static void setTabForAnimation(QTabWidget *tabWidget, const ModelNode &animation)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        QWidget *w = tabWidget->widget(i);
        if (qobject_cast<TimelineAnimationForm *>(w)->animation() == animation) {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }
}

TimelineSettingsDialog::TimelineSettingsDialog(QWidget *parent, TimelineView *view)
    : QDialog(parent)
    , m_timelineView(view)
{
    resize(520, 600);
    setModal(true);

    m_timelineSettingsModel = new TimelineSettingsModel(this, view);

    auto *timelineCornerWidget = new QToolBar;

    auto *timelineAddAction = new QAction(TimelineIcons::ADD_TIMELINE.icon(), tr("Add Timeline"));
    auto *timelineRemoveAction = new QAction(TimelineIcons::REMOVE_TIMELINE.icon(),
                                             tr("Remove Timeline"));

    connect(timelineAddAction, &QAction::triggered, this, [this]() {
        setupTimelines(m_timelineView->addNewTimeline());
    });

    connect(timelineRemoveAction, &QAction::triggered, this, [this]() {
        QmlTimeline timeline = getTimelineFromTabWidget(m_timelineTab);
        if (timeline.isValid()) {
            timeline.destroy();
            setupTimelines(QmlTimeline());
        }
    });

    timelineCornerWidget->addAction(timelineAddAction);
    timelineCornerWidget->addAction(timelineRemoveAction);

    m_timelineTab = new QTabWidget;
    m_timelineTab->setCornerWidget(timelineCornerWidget, Qt::TopRightCorner);

    auto *animationCornerWidget = new QToolBar;
    auto *animationAddAction = new QAction(TimelineIcons::ADD_TIMELINE.icon(), tr("Add Animation"));
    auto *animationRemoveAction = new QAction(TimelineIcons::REMOVE_TIMELINE.icon(),
                                              tr("Remove Animation"));

    animationCornerWidget->addAction(animationAddAction);
    animationCornerWidget->addAction(animationRemoveAction);

    connect(animationAddAction, &QAction::triggered, this, [this]() {
        setupAnimations(m_timelineView->addAnimation(m_currentTimeline));
    });

    connect(animationRemoveAction, &QAction::triggered, this, [this]() {
        ModelNode node = getAnimationFromTabWidget(m_animationTab);
        if (node.isValid()) {
            node.destroy();
            setupAnimations(m_currentTimeline);
        }
    });

    m_animationTab = new QTabWidget;
    m_animationTab->setCornerWidget(animationCornerWidget, Qt::TopRightCorner);

    m_tableView = new QTableView;
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    sp.setVerticalStretch(1);
    m_tableView->setSizePolicy(sp);

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    buttonBox->clearFocus();
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Layouting;
    Column {
        m_timelineTab,
        m_animationTab,
        m_tableView,
        buttonBox,
    }.attachTo(this);

    setupTimelines(QmlTimeline());
    setupAnimations(m_currentTimeline);

    connect(m_timelineTab, &QTabWidget::currentChanged, this, [this]() {
        m_currentTimeline = getTimelineFromTabWidget(m_timelineTab);
        setupAnimations(m_currentTimeline);
    });
    setupTableView();
}

void TimelineSettingsDialog::setCurrentTimeline(const QmlTimeline &timeline)
{
    m_currentTimeline = timeline;
    setTabForTimeline(m_timelineTab, m_currentTimeline);
}

void TimelineSettingsDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        /* ignore */
        break;

    default:
        QDialog::keyPressEvent(event);
    }
}

void TimelineSettingsDialog::setupTableView()
{
    m_tableView->setModel(m_timelineSettingsModel);
    m_timelineSettingsModel->setupDelegates(m_tableView);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->verticalHeader()->hide();
    m_tableView->setSelectionMode(QAbstractItemView::NoSelection);
    m_timelineSettingsModel->resetModel();
}

void TimelineSettingsDialog::setupTimelines(const QmlTimeline &timeline)
{
    deleteAllTabs(m_timelineTab);

    const QList<QmlTimeline> &timelines = m_timelineView->getTimelines();

    if (timelines.isEmpty()) {
        m_currentTimeline = QmlTimeline();
        auto timelineForm = new TimelineForm(this);
        timelineForm->setDisabled(true);
        m_timelineTab->addTab(timelineForm, tr("No Timeline"));
        return;
    }

    for (const auto &timeline : timelines)
        addTimelineTab(timeline);

    if (timeline.isValid()) {
        m_currentTimeline = timeline;
    } else {
        m_currentTimeline = timelines.constFirst();
    }

    setTabForTimeline(m_timelineTab, m_currentTimeline);
    setupAnimations(m_currentTimeline);
    m_timelineSettingsModel->resetModel();
}

void TimelineSettingsDialog::setupAnimations(const ModelNode &animation)
{
    deleteAllTabs(m_animationTab);

    const QList<ModelNode> animations = m_timelineView->getAnimations(m_currentTimeline);

    for (const auto &animation : animations)
        addAnimationTab(animation);

    if (animations.isEmpty()) {
        auto animationForm = new TimelineAnimationForm(this);
        animationForm->setDisabled(true);
        m_animationTab->addTab(animationForm, tr("No Animation"));
        if (currentTimelineForm())
            currentTimelineForm()->setHasAnimation(false);
    } else {
        if (currentTimelineForm())
            currentTimelineForm()->setHasAnimation(true);
    }

    if (animation.isValid())
        setTabForAnimation(m_animationTab, animation);
    m_timelineSettingsModel->resetModel();
}

void TimelineSettingsDialog::addTimelineTab(const QmlTimeline &node)
{
    auto timelineForm = new TimelineForm(this);
    m_timelineTab->addTab(timelineForm, node.modelNode().displayName());
    timelineForm->setTimeline(node);
    setupAnimations(ModelNode());
}

void TimelineSettingsDialog::addAnimationTab(const ModelNode &node)
{
    auto timelineAnimationForm = new TimelineAnimationForm(this);
    m_animationTab->addTab(timelineAnimationForm, node.displayName());
    timelineAnimationForm->setup(node);
}

TimelineForm *TimelineSettingsDialog::currentTimelineForm() const
{
    return qobject_cast<TimelineForm *>(m_timelineTab->currentWidget());
}

} // namespace QmlDesigner
