// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "transitioneditorsettingsdialog.h"
#include "timelinesettingsdialog.h"
#include "transitioneditorview.h"
#include "ui_transitioneditorsettingsdialog.h"

#include "timelineicons.h"
#include "timelinesettingsmodel.h"
#include "transitionform.h"

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

static ModelNode getTransitionFromTabWidget(QTabWidget *tabWidget)
{
    QWidget *w = tabWidget->currentWidget();
    if (w)
        return qobject_cast<TransitionForm *>(w)->transition();
    return QmlTimeline();
}

static void setTabForTransition(QTabWidget *tabWidget, const ModelNode &timeline)
{
    for (int i = 0; i < tabWidget->count(); ++i) {
        QWidget *w = tabWidget->widget(i);
        if (qobject_cast<TransitionForm *>(w)->transition() == timeline) {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }
}

TransitionEditorSettingsDialog::TransitionEditorSettingsDialog(QWidget *parent,
                                                               class TransitionEditorView *view)
    : QDialog(parent)
    , ui(new Ui::TransitionEditorSettingsDialog)
    , m_transitionEditorView(view)
{
    //m_timelineSettingsModel = new TimelineSettingsModel(this, view);

    ui->setupUi(this);

    auto *transitionCornerWidget = new QToolBar;

    auto *transitionAddAction = new QAction(TimelineIcons::ADD_TIMELINE.icon(),
                                            tr("Add Transition"));
    auto *transitionRemoveAction = new QAction(TimelineIcons::REMOVE_TIMELINE.icon(),
                                               tr("Remove Transition"));

    connect(transitionAddAction, &QAction::triggered, this, [this]() {
        setupTransitions(m_transitionEditorView->addNewTransition());
    });

    connect(transitionRemoveAction, &QAction::triggered, this, [this]() {
        if (ModelNode transition = getTransitionFromTabWidget(ui->timelineTab)) {
            transition.destroy();
            setupTransitions({});
        }
    });

    transitionCornerWidget->addAction(transitionAddAction);
    transitionCornerWidget->addAction(transitionRemoveAction);

    ui->timelineTab->setCornerWidget(transitionCornerWidget, Qt::TopRightCorner);

    setupTransitions({});

    connect(ui->timelineTab, &QTabWidget::currentChanged, this, [this]() {
        m_currentTransition = getTransitionFromTabWidget(ui->timelineTab);
    });
}

void TransitionEditorSettingsDialog::setCurrentTransition(const ModelNode &timeline)
{
    m_currentTransition = timeline;
    setTabForTransition(ui->timelineTab, m_currentTransition);
}

TransitionEditorSettingsDialog::~TransitionEditorSettingsDialog()
{
    delete ui;
}

void TransitionEditorSettingsDialog::keyPressEvent(QKeyEvent *event)
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

void TransitionEditorSettingsDialog::setupTransitions(const ModelNode &newTransition)
{
    deleteAllTabs(ui->timelineTab);

    const QList<ModelNode> &transitions = m_transitionEditorView->allTransitions();

    if (transitions.isEmpty()) {
        m_currentTransition = {};
        auto transitionForm = new TransitionForm(this);
        transitionForm->setDisabled(true);
        ui->timelineTab->addTab(transitionForm, tr("No Transition"));
        return;
    }

    for (const auto &transition : transitions)
        addTransitionTab(transition);

    if (newTransition.isValid()) {
        m_currentTransition = newTransition;
    } else {
        m_currentTransition = transitions.constFirst();
    }

    setTabForTransition(ui->timelineTab, m_currentTransition);
}

void TransitionEditorSettingsDialog::addTransitionTab(const QmlTimeline &node)
{
    auto transitionForm = new TransitionForm(this);
    ui->timelineTab->addTab(transitionForm, node.modelNode().displayName());
    transitionForm->setTransition(node);
}

} // namespace QmlDesigner
