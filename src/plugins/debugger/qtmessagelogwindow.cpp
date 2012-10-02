/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qtmessagelogwindow.h"
#include "qtmessagelogview.h"
#include "qtmessageloghandler.h"
#include "qtmessagelogitemdelegate.h"
#include "debuggerstringutils.h"
#include "qtmessagelogproxymodel.h"

#include <utils/statuslabel.h>
#include <utils/styledbar.h>
#include <utils/savedaction.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/findplaceholder.h>

#include <aggregation/aggregate.h>
#include <find/treeviewfind.h>

#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>

static const char CONSOLE[] = "Console";
static const char SHOW_LOG[] = "showLog";
static const char SHOW_WARNING[] = "showWarning";
static const char SHOW_ERROR[] = "showError";

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////
//
// QtMessageLogWindow
//
/////////////////////////////////////////////////////////////////////

QtMessageLogWindow::QtMessageLogWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr(CONSOLE));
    setObjectName(_(CONSOLE));

    const int statusBarHeight = 25;

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    QWidget *statusbarContainer = new Utils::StyledBar();
    statusbarContainer->setStyleSheet(QLatin1String("background: #9B9B9B"));
    statusbarContainer->setFixedHeight(statusBarHeight);
    QHBoxLayout *hbox = new QHBoxLayout(statusbarContainer);
    hbox->setMargin(0);
    hbox->setSpacing(0);

    hbox->addSpacing(5);

    //Status Label
    m_statusLabel = new Utils::StatusLabel;
    hbox->addWidget(m_statusLabel);
    hbox->addWidget(new Utils::StyledSeparator);

    const int buttonWidth = 25;
    //Filters
    QToolButton *button = new QToolButton(this);
    button->setAutoRaise(true);
    button->setFixedWidth(buttonWidth);
    m_showLogAction = new Utils::SavedAction(this);
    m_showLogAction->setDefaultValue(true);
    m_showLogAction->setSettingsKey(_(CONSOLE), _(SHOW_LOG));
    m_showLogAction->setText(tr("Log"));
    m_showLogAction->setToolTip(tr("Show debug, log, and info messages."));
    m_showLogAction->setCheckable(true);
    m_showLogAction->setIcon(QIcon(_(":/debugger/images/log.png")));
    button->setDefaultAction(m_showLogAction);
    hbox->addWidget(button);

    button = new QToolButton(this);
    button->setAutoRaise(true);
    button->setFixedWidth(buttonWidth);
    m_showWarningAction = new Utils::SavedAction(this);
    m_showWarningAction->setDefaultValue(true);
    m_showWarningAction->setSettingsKey(_(CONSOLE), _(SHOW_WARNING));
    m_showWarningAction->setText(tr("Warning"));
    m_showWarningAction->setToolTip(tr("Show warning messages."));
    m_showWarningAction->setCheckable(true);
    m_showWarningAction->setIcon(QIcon(_(":/debugger/images/warning.png")));
    button->setDefaultAction(m_showWarningAction);
    hbox->addWidget(button);

    button = new QToolButton(this);
    button->setAutoRaise(true);
    button->setFixedWidth(buttonWidth);
    m_showErrorAction = new Utils::SavedAction(this);
    m_showErrorAction->setDefaultValue(true);
    m_showErrorAction->setSettingsKey(_(CONSOLE), _(SHOW_ERROR));
    m_showErrorAction->setText(tr("Error"));
    m_showErrorAction->setToolTip(tr("Show error and fatal messages."));
    m_showErrorAction->setCheckable(true);
    m_showErrorAction->setIcon(QIcon(_(":/debugger/images/error.png")));
    button->setDefaultAction(m_showErrorAction);
    hbox->addWidget(button);
    hbox->addWidget(new Utils::StyledSeparator);

    //Clear Button
    button = new QToolButton;
    button->setAutoRaise(true);
    button->setFixedWidth(buttonWidth);
    m_clearAction = new QAction(tr("Clear Console"), this);
    m_clearAction->setIcon(QIcon(_(Core::Constants::ICON_CLEAN_PANE)));
    button->setDefaultAction(m_clearAction);
    hbox->addWidget(button);
    hbox->addWidget(new Utils::StyledSeparator);

    m_treeView = new QtMessageLogView(this);
    m_treeView->setSizePolicy(QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding);

    m_proxyModel = new QtMessageLogProxyModel(this);
    connect(m_showLogAction, SIGNAL(toggled(bool)),
            m_proxyModel, SLOT(setShowLogs(bool)));
    connect(m_showWarningAction, SIGNAL(toggled(bool)),
            m_proxyModel, SLOT(setShowWarnings(bool)));
    connect(m_showErrorAction, SIGNAL(toggled(bool)),
            m_proxyModel, SLOT(setShowErrors(bool)));

    m_treeView->setModel(m_proxyModel);
    connect(m_proxyModel,
            SIGNAL(setCurrentIndex(QModelIndex,QItemSelectionModel::SelectionFlags)),
            m_treeView->selectionModel(),
            SLOT(setCurrentIndex(QModelIndex,QItemSelectionModel::SelectionFlags)));
    connect(m_proxyModel,
            SIGNAL(scrollToBottom()),
            m_treeView,
            SLOT(onScrollToBottom()));

    m_itemDelegate = new QtMessageLogItemDelegate(this);
    connect(m_treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            m_itemDelegate, SLOT(currentChanged(QModelIndex,QModelIndex)));
    m_treeView->setItemDelegate(m_itemDelegate);

    vbox->addWidget(statusbarContainer);
    vbox->addWidget(m_treeView);
    vbox->addWidget(new Core::FindToolBarPlaceHolder(this));

    readSettings();
    connect(Core::ICore::instance(),
            SIGNAL(saveSettingsRequested()), SLOT(writeSettings()));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate();
    aggregate->add(m_treeView);
    aggregate->add(new Find::TreeViewFind(m_treeView));
}

QtMessageLogWindow::~QtMessageLogWindow()
{
    writeSettings();
}

void QtMessageLogWindow::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    m_showLogAction->readSettings(settings);
    m_showWarningAction->readSettings(settings);
    m_showErrorAction->readSettings(settings);
}

void QtMessageLogWindow::showStatus(const QString &context, int timeout)
{
    m_statusLabel->showStatusMessage(context, timeout);
}

void QtMessageLogWindow::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    m_showLogAction->writeSettings(settings);
    m_showWarningAction->writeSettings(settings);
    m_showErrorAction->writeSettings(settings);
}

void QtMessageLogWindow::setModel(QAbstractItemModel *model)
{
    QtMessageLogHandler *oldHandler = qobject_cast<QtMessageLogHandler *>(
                m_proxyModel->sourceModel());
    if (oldHandler) {
        disconnect(m_clearAction, SIGNAL(triggered()), oldHandler, SLOT(clear()));
        disconnect(oldHandler,
                SIGNAL(selectEditableRow(
                           QModelIndex,QItemSelectionModel::SelectionFlags)),
                m_proxyModel,
                SLOT(selectEditableRow(
                         QModelIndex,QItemSelectionModel::SelectionFlags)));
        disconnect(oldHandler,
                SIGNAL(rowsInserted(QModelIndex,int,int)),
                m_proxyModel,
                SLOT(onRowsInserted(QModelIndex,int,int)));
    }

    QtMessageLogHandler *newHandler = qobject_cast<QtMessageLogHandler *>(model);
    m_proxyModel->setSourceModel(newHandler);
    m_itemDelegate->setItemModel(newHandler);

    if (newHandler) {
        connect(m_clearAction, SIGNAL(triggered()), newHandler, SLOT(clear()));
        connect(newHandler,
                SIGNAL(selectEditableRow(
                           QModelIndex,QItemSelectionModel::SelectionFlags)),
                m_proxyModel,
                SLOT(selectEditableRow(
                         QModelIndex,QItemSelectionModel::SelectionFlags)));

        //Scroll to bottom when rows matching current filter settings are inserted
        //Not connecting rowsRemoved as the only way to remove rows is to clear the
        //model which will automatically reset the view.
        connect(newHandler,
                SIGNAL(rowsInserted(QModelIndex,int,int)),
                m_proxyModel,
                SLOT(onRowsInserted(QModelIndex,int,int)));
    }
}

} // namespace Internal
} // namespace Debugger
