/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "timelinesettingsdialog.h"
#include "ui_timelinesettingsdialog.h"

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
#include <utils/qtcassert.h>

#include <QKeyEvent>
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
    , ui(new Ui::TimelineSettingsDialog)
    , m_timelineView(view)
{
    m_timelineSettingsModel = new TimelineSettingsModel(this, view);

    ui->setupUi(this);

    auto *timelineCornerWidget = new QToolBar;

    auto *timelineAddAction = new QAction(TimelineIcons::ADD_TIMELINE.icon(), tr("Add Timeline"));
    auto *timelineRemoveAction = new QAction(TimelineIcons::REMOVE_TIMELINE.icon(),
                                             tr("Remove Timeline"));

    connect(timelineAddAction, &QAction::triggered, this, [this]() {
        setupTimelines(m_timelineView->addNewTimeline());
    });

    connect(timelineRemoveAction, &QAction::triggered, this, [this]() {
        QmlTimeline timeline = getTimelineFromTabWidget(ui->timelineTab);
        if (timeline.isValid()) {
            timeline.destroy();
            setupTimelines(QmlTimeline());
        }
    });

    timelineCornerWidget->addAction(timelineAddAction);
    timelineCornerWidget->addAction(timelineRemoveAction);

    ui->timelineTab->setCornerWidget(timelineCornerWidget, Qt::TopRightCorner);

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
        ModelNode node = getAnimationFromTabWidget(ui->animationTab);
        if (node.isValid()) {
            node.destroy();
            setupAnimations(m_currentTimeline);
        }
    });

    ui->animationTab->setCornerWidget(animationCornerWidget, Qt::TopRightCorner);
    ui->buttonBox->clearFocus();

    setupTimelines(QmlTimeline());
    setupAnimations(m_currentTimeline);

    connect(ui->timelineTab, &QTabWidget::currentChanged, this, [this]() {
        m_currentTimeline = getTimelineFromTabWidget(ui->timelineTab);
        setupAnimations(m_currentTimeline);
    });
    setupTableView();
}

void TimelineSettingsDialog::setCurrentTimeline(const QmlTimeline &timeline)
{
    m_currentTimeline = timeline;
    setTabForTimeline(ui->timelineTab, m_currentTimeline);
}

TimelineSettingsDialog::~TimelineSettingsDialog()
{
    delete ui;
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
    ui->tableView->setModel(m_timelineSettingsModel);
    m_timelineSettingsModel->setupDelegates(ui->tableView);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->verticalHeader()->hide();
    ui->tableView->setSelectionMode(QAbstractItemView::NoSelection);
    m_timelineSettingsModel->resetModel();
}

void TimelineSettingsDialog::setupTimelines(const QmlTimeline &timeline)
{
    deleteAllTabs(ui->timelineTab);

    const QList<QmlTimeline> &timelines = m_timelineView->getTimelines();

    if (timelines.isEmpty()) {
        m_currentTimeline = QmlTimeline();
        auto timelineForm = new TimelineForm(this);
        timelineForm->setDisabled(true);
        ui->timelineTab->addTab(timelineForm, tr("No Timeline"));
        return;
    }

    for (const auto &timeline : timelines)
        addTimelineTab(timeline);

    if (timeline.isValid()) {
        m_currentTimeline = timeline;
    } else {
        m_currentTimeline = timelines.constFirst();
    }

    setTabForTimeline(ui->timelineTab, m_currentTimeline);
    setupAnimations(m_currentTimeline);
    m_timelineSettingsModel->resetModel();
}

void TimelineSettingsDialog::setupAnimations(const ModelNode &animation)
{
    deleteAllTabs(ui->animationTab);

    const QList<ModelNode> animations = m_timelineView->getAnimations(m_currentTimeline);

    for (const auto &animation : animations)
        addAnimationTab(animation);

    if (animations.isEmpty()) {
        auto animationForm = new TimelineAnimationForm(this);
        animationForm->setDisabled(true);
        ui->animationTab->addTab(animationForm, tr("No Animation"));
        if (currentTimelineForm())
            currentTimelineForm()->setHasAnimation(false);
    } else {
        if (currentTimelineForm())
            currentTimelineForm()->setHasAnimation(true);
    }

    if (animation.isValid())
        setTabForAnimation(ui->animationTab, animation);
    m_timelineSettingsModel->resetModel();
}

void TimelineSettingsDialog::addTimelineTab(const QmlTimeline &node)
{
    auto timelineForm = new TimelineForm(this);
    ui->timelineTab->addTab(timelineForm, node.modelNode().displayName());
    timelineForm->setTimeline(node);
    setupAnimations(ModelNode());
}

void TimelineSettingsDialog::addAnimationTab(const ModelNode &node)
{
    auto timelineAnimationForm = new TimelineAnimationForm(this);
    ui->animationTab->addTab(timelineAnimationForm, node.displayName());
    timelineAnimationForm->setup(node);
}

TimelineForm *TimelineSettingsDialog::currentTimelineForm() const
{
    return qobject_cast<TimelineForm *>(ui->timelineTab->currentWidget());
}

} // namespace QmlDesigner
