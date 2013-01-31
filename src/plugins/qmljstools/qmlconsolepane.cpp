/**************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlconsolepane.h"
#include "qmlconsoleview.h"
#include "qmlconsoleproxymodel.h"
#include "qmlconsoleitemmodel.h"
#include "qmlconsolemanager.h"
#include "qmlconsoleitemdelegate.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/findplaceholder.h>
#include <utils/savedaction.h>
#include <aggregation/aggregate.h>
#include <find/treeviewfind.h>

#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>

static const char CONSOLE[] = "Console";
static const char SHOW_LOG[] = "showLog";
static const char SHOW_WARNING[] = "showWarning";
static const char SHOW_ERROR[] = "showError";

namespace QmlJSTools {
namespace Internal {

/////////////////////////////////////////////////////////////////////
//
// QmlConsolePane
//
/////////////////////////////////////////////////////////////////////

QmlConsolePane::QmlConsolePane(QObject *parent)
    : Core::IOutputPane(parent)
{
    m_consoleWidget = new QWidget;
    m_consoleWidget->setWindowTitle(displayName());
    m_consoleWidget->setEnabled(true);

    QVBoxLayout *vbox = new QVBoxLayout(m_consoleWidget);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    m_consoleView = new QmlConsoleView(m_consoleWidget);
    m_proxyModel = new QmlConsoleProxyModel(this);
    m_proxyModel->setSourceModel(QmlConsoleModel::qmlConsoleItemModel());
    connect(QmlConsoleModel::qmlConsoleItemModel(),
            SIGNAL(selectEditableRow(QModelIndex, QItemSelectionModel::SelectionFlags)),
            m_proxyModel,
            SLOT(selectEditableRow(QModelIndex,QItemSelectionModel::SelectionFlags)));

    //Scroll to bottom when rows matching current filter settings are inserted
    //Not connecting rowsRemoved as the only way to remove rows is to clear the
    //model which will automatically reset the view.
    connect(QmlConsoleModel::qmlConsoleItemModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            m_proxyModel, SLOT(onRowsInserted(QModelIndex,int,int)));
    m_consoleView->setModel(m_proxyModel);

    connect(m_proxyModel,
            SIGNAL(setCurrentIndex(QModelIndex,QItemSelectionModel::SelectionFlags)),
            m_consoleView->selectionModel(),
            SLOT(setCurrentIndex(QModelIndex,QItemSelectionModel::SelectionFlags)));
    connect(m_proxyModel, SIGNAL(scrollToBottom()), m_consoleView, SLOT(onScrollToBottom()));

    m_itemDelegate = new QmlConsoleItemDelegate(this);
    connect(m_consoleView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            m_itemDelegate, SLOT(currentChanged(QModelIndex,QModelIndex)));
    m_consoleView->setItemDelegate(m_itemDelegate);

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate();
    aggregate->add(m_consoleView);
    aggregate->add(new Find::TreeViewFind(m_consoleView));

    vbox->addWidget(m_consoleView);
    vbox->addWidget(new Core::FindToolBarPlaceHolder(m_consoleWidget));

    m_showDebugButton = new QToolButton(m_consoleWidget);
    m_showDebugButton->setAutoRaise(true);

    m_showDebugButtonAction = new Utils::SavedAction(this);
    m_showDebugButtonAction->setDefaultValue(true);
    m_showDebugButtonAction->setSettingsKey(QLatin1String(CONSOLE), QLatin1String(SHOW_LOG));
    m_showDebugButtonAction->setToolTip(tr("Show debug, log, and info messages."));
    m_showDebugButtonAction->setCheckable(true);
    m_showDebugButtonAction->setIcon(QIcon(QLatin1String(":/qmljstools/images/log.png")));
    connect(m_showDebugButtonAction, SIGNAL(toggled(bool)), m_proxyModel, SLOT(setShowLogs(bool)));
    m_showDebugButton->setDefaultAction(m_showDebugButtonAction);

    m_showWarningButton = new QToolButton(m_consoleWidget);
    m_showWarningButton->setAutoRaise(true);

    m_showWarningButtonAction = new Utils::SavedAction(this);
    m_showWarningButtonAction->setDefaultValue(true);
    m_showWarningButtonAction->setSettingsKey(QLatin1String(CONSOLE), QLatin1String(SHOW_WARNING));
    m_showWarningButtonAction->setToolTip(tr("Show debug, log, and info messages."));
    m_showWarningButtonAction->setCheckable(true);
    m_showWarningButtonAction->setIcon(QIcon(QLatin1String(":/qmljstools/images/warning.png")));
    connect(m_showWarningButtonAction, SIGNAL(toggled(bool)), m_proxyModel,
            SLOT(setShowWarnings(bool)));
    m_showWarningButton->setDefaultAction(m_showWarningButtonAction);

    m_showErrorButton = new QToolButton(m_consoleWidget);
    m_showErrorButton->setAutoRaise(true);

    m_showErrorButtonAction = new Utils::SavedAction(this);
    m_showErrorButtonAction->setDefaultValue(true);
    m_showErrorButtonAction->setSettingsKey(QLatin1String(CONSOLE), QLatin1String(SHOW_ERROR));
    m_showErrorButtonAction->setToolTip(tr("Show debug, log, and info messages."));
    m_showErrorButtonAction->setCheckable(true);
    m_showErrorButtonAction->setIcon(QIcon(QLatin1String(":/qmljstools/images/error.png")));
    connect(m_showErrorButtonAction, SIGNAL(toggled(bool)), m_proxyModel,
            SLOT(setShowErrors(bool)));
    m_showErrorButton->setDefaultAction(m_showErrorButtonAction);

    m_spacer = new QWidget(m_consoleWidget);
    m_spacer->setMinimumWidth(30);

    m_statusLabel = new QLabel(m_consoleWidget);

    readSettings();
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), SLOT(writeSettings()));
}

QmlConsolePane::~QmlConsolePane()
{
    writeSettings();
    delete m_consoleWidget;
}

QWidget *QmlConsolePane::outputWidget(QWidget *)
{
    return m_consoleWidget;
}

QList<QWidget *> QmlConsolePane::toolBarWidgets() const
{
     return QList<QWidget *>() << m_showDebugButton << m_showWarningButton << m_showErrorButton
                               << m_spacer << m_statusLabel;
}

int QmlConsolePane::priorityInStatusBar() const
{
    return 20;
}

void QmlConsolePane::clearContents()
{
    QmlConsoleModel::qmlConsoleItemModel()->clear();
}

void QmlConsolePane::visibilityChanged(bool /*visible*/)
{
}

bool QmlConsolePane::canFocus() const
{
    return true;
}

bool QmlConsolePane::hasFocus() const
{
    return m_consoleWidget->hasFocus();
}

void QmlConsolePane::setFocus()
{
    m_consoleWidget->setFocus();
}

bool QmlConsolePane::canNext() const
{
    return false;
}

bool QmlConsolePane::canPrevious() const
{
    return false;
}

void QmlConsolePane::goToNext()
{
}

void QmlConsolePane::goToPrev()
{
}

bool QmlConsolePane::canNavigate() const
{
    return false;
}

void QmlConsolePane::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    m_showDebugButtonAction->readSettings(settings);
    m_showWarningButtonAction->readSettings(settings);
    m_showErrorButtonAction->readSettings(settings);
}

void QmlConsolePane::setContext(const QString &context)
{
    m_statusLabel->setText(context);
}

void QmlConsolePane::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    m_showDebugButtonAction->writeSettings(settings);
    m_showWarningButtonAction->writeSettings(settings);
    m_showErrorButtonAction->writeSettings(settings);
}

} // Internal
} // QmlJSTools
