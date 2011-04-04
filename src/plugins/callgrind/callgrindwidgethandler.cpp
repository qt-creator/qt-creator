/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "callgrindwidgethandler.h"

#include "callgrindcostview.h"
#include "callgrindengine.h"
#include "callgrindvisualisation.h"

#ifndef DISABLE_CALLGRIND_WORKAROUNDS
#include "workarounds.h"
#endif

#include <analyzerbase/analyzerconstants.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>

#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindparsedata.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindcostitem.h>
#include <valgrind/callgrind/callgrindstackbrowser.h>
#include <valgrind/callgrind/callgrindcallmodel.h>
#include <valgrind/callgrind/callgrindproxymodel.h>
#include <valgrind/callgrind/callgrindfunctioncall.h>

#include <utils/fancymainwindow.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QGraphicsItem>
#include <QtGui/QToolButton>
#include <QtGui/QAction>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QActionGroup>
#include <QSortFilterProxyModel>
#include <QComboBox>

using namespace Valgrind::Callgrind;

namespace Callgrind {
namespace Internal {

CallgrindWidgetHandler::CallgrindWidgetHandler(QWidget *parent)
: QObject(parent)
, m_dataModel(new DataModel(this))
, m_dataProxy(new DataProxyModel(this))
, m_stackBrowser(new StackBrowser(this))
, m_callersModel(new CallModel(this))
, m_calleesModel(new CallModel(this))
, m_flatView(0)
, m_callersView(0)
, m_calleesView(0)
, m_visualisation(0)
, m_goToOverview(0)
, m_goBack(0)
, m_searchFilter(0)
, m_filterProjectCosts(0)
, m_costAbsolute(0)
, m_costRelative(0)
, m_costRelativeToParent(0)
, m_eventCombo(0)
, m_updateTimer(new QTimer(this))
{
    connect(m_stackBrowser, SIGNAL(currentChanged()), this, SLOT(stackBrowserChanged()));

    m_updateTimer->setInterval(200);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, SIGNAL(timeout()), SLOT(updateFilterString()));

    m_dataProxy->setSourceModel(m_dataModel);
    m_dataProxy->setDynamicSortFilter(true);
    m_dataProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_dataProxy->setFilterKeyColumn(DataModel::NameColumn);
    m_dataProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_flatView = new CostView(parent);
    m_flatView->sortByColumn(DataModel::SelfCostColumn);
    m_flatView->setFrameStyle(QFrame::NoFrame);
    m_flatView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_flatView->setModel(m_dataProxy);
    m_flatView->setObjectName("Valgrind.CallgrindWidgetHandler.FlatView");
    connect(m_flatView, SIGNAL(activated(QModelIndex)),
            this, SLOT(dataFunctionSelected(QModelIndex)));

    m_visualisation = new Visualisation(parent);
    m_visualisation->setObjectName("Valgrind.CallgrindWidgetHandler.Visualisation");
    m_visualisation->setModel(m_dataModel);
    connect(m_visualisation, SIGNAL(functionActivated(const Valgrind::Callgrind::Function*)),
            this, SLOT(visualisationFunctionSelected(const Valgrind::Callgrind::Function*)));

    {
    m_calleesView = new CostView(parent);
    m_calleesView->sortByColumn(CallModel::CostColumn);
    m_calleesView->setObjectName("Valgrind.CallgrindWidgetHandler.CalleesView");
    // enable sorting
    QSortFilterProxyModel *calleeProxy = new QSortFilterProxyModel(m_calleesModel);
    calleeProxy->setSourceModel(m_calleesModel);
    m_calleesView->setModel(calleeProxy);
    m_calleesView->hideColumn(CallModel::CallerColumn);
    connect(m_calleesView, SIGNAL(activated(QModelIndex)),
            this, SLOT(calleeFunctionSelected(QModelIndex)));
    }
    {
    m_callersView = new CostView(parent);
    m_callersView->sortByColumn(CallModel::CostColumn);
    m_callersView->setObjectName("Valgrind.CallgrindWidgetHandler.CallersView");
    // enable sorting
    QSortFilterProxyModel *callerProxy = new QSortFilterProxyModel(m_callersModel);
    callerProxy->setSourceModel(m_callersModel);
    m_callersView->setModel(callerProxy);
    m_callersView->hideColumn(CallModel::CalleeColumn);
    connect(m_callersView, SIGNAL(activated(QModelIndex)),
            this, SLOT(callerFunctionSelected(QModelIndex)));
    }
}

void CallgrindWidgetHandler::populateActions(QLayout *layout)
{
    // navigation
    {
    QToolButton *button;
    QAction *action;

    // go back
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon::fromTheme("go-previous"));
    action->setText(tr("Back"));
    action->setToolTip(tr("Go back one step in history. This will select the previously selected item."));
    m_goBack = action;
    connect(action, SIGNAL(triggered(bool)), m_stackBrowser, SLOT(goBack()));
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);

    // overview
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon::fromTheme("go-up"));
    action->setText(tr("All Functions"));
    action->setToolTip(tr("Show the overview of all function calls."));
    m_goToOverview = action;
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotGoToOverview()));
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    }

    layout->addWidget(new Utils::StyledSeparator);

    // event selection
    {
    m_eventCombo = new QComboBox;
    m_eventCombo->setToolTip(tr("Selects which events from the profiling data are shown and visualized."));
    connect(m_eventCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setCostEvent(int)));
    updateEventCombo();
    layout->addWidget(m_eventCombo);
    }

    // cost formatting
    {
    QMenu *menu = new QMenu(layout->parentWidget());
    QActionGroup *group = new QActionGroup(this);

    // show costs as absolute numbers
    m_costAbsolute = new QAction(tr("Absolute Costs"), this);
    ///FIXME: icon
    m_costAbsolute->setToolTip(tr("Show costs as absolute numbers."));
    m_costAbsolute->setCheckable(true);
    m_costAbsolute->setChecked(true);
    connect(m_costAbsolute, SIGNAL(toggled(bool)),
            this, SLOT(updateCostFormat()));
    group->addAction(m_costAbsolute);
    menu->addAction(m_costAbsolute);

    // show costs in percentages
    m_costRelative = new QAction(tr("Relative Costs"), this);
    ///FIXME: icon (percentage sign?)
    m_costRelative->setToolTip(tr("Show costs relative to total inclusive cost."));
    m_costRelative->setCheckable(true);
    connect(m_costRelative, SIGNAL(toggled(bool)),
            this, SLOT(updateCostFormat()));
    group->addAction(m_costRelative);
    menu->addAction(m_costRelative);

    // show costs relative to parent
    m_costRelativeToParent = new QAction(tr("Relative Costs To Parent"), this);
    ///FIXME: icon
    m_costRelativeToParent->setToolTip(tr("Show costs relative to parent functions inclusive cost."));
    m_costRelativeToParent->setCheckable(true);
    connect(m_costRelativeToParent, SIGNAL(toggled(bool)),
            this, SLOT(updateCostFormat()));
    group->addAction(m_costRelativeToParent);
    menu->addAction(m_costRelativeToParent);

    QToolButton *button = new QToolButton;
    button->setMenu(menu);
    button->setPopupMode(QToolButton::InstantPopup);
    button->setText(tr("Cost Format"));
    layout->addWidget(button);
    }

    {
    // cycle detection
    m_cycleDetection = new QAction(tr("Cycle Detection"), this);
    ///FIXME: icon
    m_cycleDetection->setToolTip(tr("Enable cycle detection to properly handle recursive or circular function calls."));
    connect(m_cycleDetection, SIGNAL(toggled(bool)),
            m_dataModel, SLOT(enableCycleDetection(bool)));
    connect(m_cycleDetection, SIGNAL(toggled(bool)),
            this, SIGNAL(cycleDetectionEnabled(bool)));
    m_cycleDetection->setCheckable(true);
    QToolButton *button = new QToolButton;
    button->setDefaultAction(m_cycleDetection);
    layout->addWidget(button);
    }

    // filtering
    {
    QToolButton *button;
    m_filterProjectCosts = new QAction(tr("Show Project Costs Only"), this);
    m_filterProjectCosts->setIcon(QIcon(Core::Constants::ICON_FILTER));
    m_filterProjectCosts->setToolTip(tr("Show only profiling info that originated from this project source."));
    m_filterProjectCosts->setCheckable(true);
    connect(m_filterProjectCosts, SIGNAL(toggled(bool)),
            this, SLOT( handleFilterProjectCosts()));
    button = new QToolButton;
    button->setDefaultAction(m_filterProjectCosts);
    layout->addWidget(button);

    // filter
    ///FIXME: find workaround for http://bugreports.qt.nokia.com/browse/QTCREATORBUG-3247
    QLineEdit *filter = new QLineEdit;
    filter->setPlaceholderText(tr("Filter..."));
    connect(filter, SIGNAL(textChanged(QString)),
            m_updateTimer, SLOT(start()));
    layout->addWidget(filter);
    m_searchFilter = filter;
    }

    layout->addWidget(new Utils::StyledSeparator);
}

CallgrindWidgetHandler::~CallgrindWidgetHandler()
{
    slotClear();
}

void CallgrindWidgetHandler::slotGoToOverview()
{
    selectFunction(0);
}

void CallgrindWidgetHandler::slotClear()
{
    setParseData(0);

    // clear filters
    m_filterProjectCosts->setChecked(false);
    m_dataProxy->setFilterBaseDir(QString());
    m_searchFilter->setText(QString());
    m_dataProxy->setFilterFixedString(QString());
}

void CallgrindWidgetHandler::slotRequestDump()
{
    m_visualisation->setText(tr("Populating..."));
    emit dumpRequested();
}

void CallgrindWidgetHandler::selectFunction(const Function *func)
{
    if (!func) {
        m_goToOverview->setDisabled(true);

        m_flatView->clearSelection();
        m_visualisation->setFunction(0);
        m_callersModel->clear();
        m_calleesModel->clear();

        emit functionSelected(0);
        return;
    }

    m_goToOverview->setEnabled(true);

    const QModelIndex index = m_dataModel->indexForObject(func);
    const QModelIndex proxyIndex = m_dataProxy->mapFromSource(index);
    m_flatView->selectionModel()->clearSelection();
    m_flatView->selectionModel()->setCurrentIndex(proxyIndex,
                                                  QItemSelectionModel::ClearAndSelect |
                                                  QItemSelectionModel::Rows);
    m_flatView->scrollTo(proxyIndex);

    m_callersModel->setCalls(func->incomingCalls(), func);
    m_calleesModel->setCalls(func->outgoingCalls(), func);
    m_visualisation->setFunction(func);

    FunctionHistoryItem *item = dynamic_cast<FunctionHistoryItem *>(m_stackBrowser->current());
    if (!item || item->function() != func)
        m_stackBrowser->select(new FunctionHistoryItem(func));

    emit functionSelected(func);
}

void CallgrindWidgetHandler::stackBrowserChanged()
{
    FunctionHistoryItem *item = dynamic_cast<FunctionHistoryItem *>(m_stackBrowser->current());

    if (!item || item->function() == 0)
        m_goBack->setDisabled(true);
    else
        m_goBack->setEnabled(true);

    selectFunction(item ? item->function() : 0);
}

void CallgrindWidgetHandler::updateFilterString()
{
    m_dataProxy->setFilterFixedString(m_searchFilter->text());
}

void CallgrindWidgetHandler::setCostFormat(CostDelegate::CostFormat format)
{
    switch(format) {
        case CostDelegate::FormatAbsolute:
            m_costAbsolute->setChecked(true);
            break;
        case CostDelegate::FormatRelative:
            m_costRelative->setChecked(true);
            break;
        case CostDelegate::FormatRelativeToParent:
            m_costRelativeToParent->setChecked(true);
            break;
    }
}

void CallgrindWidgetHandler::setCostEvent(int index)
{
    // prevent assert in model, don't try to set event to -1
    // happens when we clear the eventcombo
    if (index == -1)
        index = 0;

    m_dataModel->setCostEvent(index);
    m_calleesModel->setCostEvent(index);
    m_callersModel->setCostEvent(index);
}

void CallgrindWidgetHandler::enableCycleDetection(bool enabled)
{
    m_cycleDetection->setChecked(enabled);
}

void CallgrindWidgetHandler::updateCostFormat()
{
    CostDelegate::CostFormat format = CostDelegate::FormatAbsolute;

    if (m_costRelativeToParent->isChecked())
        format = CostDelegate::FormatRelativeToParent;
    else if (m_costRelative->isChecked())
        format = CostDelegate::FormatRelative;
    // else = Absolute

    m_flatView->setCostFormat(format);
    m_calleesView->setCostFormat(format);
    m_callersView->setCostFormat(format);

    emit costFormatChanged(format);
}

void CallgrindWidgetHandler::handleFilterProjectCosts()
{
    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *pro = pe->currentProject();
    QTC_ASSERT(pro, return)

    if (m_filterProjectCosts->isChecked()) {
        const QString projectDir = pro->projectDirectory();
        m_dataProxy->setFilterBaseDir(projectDir);
    }
    else {
        m_dataProxy->setFilterBaseDir(QString());
    }
}

void CallgrindWidgetHandler::dataFunctionSelected(const QModelIndex &index)
{
    const Function *func = index.data(DataModel::FunctionRole).value<const Function *>();
    QTC_ASSERT(func, return);

    selectFunction(func);
}

void CallgrindWidgetHandler::calleeFunctionSelected(const QModelIndex &index)
{
    const FunctionCall *call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
    QTC_ASSERT(call, return);

    selectFunction(call->callee());
}

void CallgrindWidgetHandler::callerFunctionSelected(const QModelIndex &index)
{
    const FunctionCall *call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
    QTC_ASSERT(call, return);

    selectFunction(call->caller());
}

void CallgrindWidgetHandler::visualisationFunctionSelected(const Function *function)
{
    if (function && function == m_visualisation->function())
        // up-navigation when the initial function was activated
        m_stackBrowser->goBack();
    else
        selectFunction(function);
}

void CallgrindWidgetHandler::setParseData(ParseData *data)
{
    // we have new parse data, invalidate filters in the proxy model
    m_visualisation->setFunction(0);

    // invalidate parse data in the data model
    delete m_dataModel->parseData();

    if (data && data->events().isEmpty()) {
        // might happen if the user cancelled the profile run
        // callgrind then sometimes produces empty callgrind.out.PID files
        delete data;
        data = 0;
    }
    m_dataModel->setParseData(data);
    m_calleesModel->setParseData(data);
    m_callersModel->setParseData(data);

    updateEventCombo();

    // clear history for new data
    m_stackBrowser->clear();
}

void CallgrindWidgetHandler::updateEventCombo()
{
    QTC_ASSERT(m_eventCombo, return)

    m_eventCombo->clear();

    const ParseData *data = m_dataModel->parseData();
    if (!data || data->events().isEmpty()) {
        m_eventCombo->hide();
        return;
    }

    m_eventCombo->show();
    foreach(const QString &event, data->events())
        m_eventCombo->addItem(ParseData::prettyStringForEvent(event));
}

}
}
