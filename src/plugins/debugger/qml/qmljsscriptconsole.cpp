/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsscriptconsole.h"
#include "qmladapter.h"
#include "debuggerstringutils.h"
#include "consoletreeview.h"
#include "consoleitemmodel.h"
#include "consoleitemdelegate.h"
#include "consolebackend.h"
#include "debuggerstringutils.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/coreconstants.h>
#include <utils/statuslabel.h>
#include <utils/styledbar.h>
#include <utils/savedaction.h>

#include <coreplugin/icore.h>

#include <qmljsdebugclient/qdebugmessageclient.h>
#include <debugger/qml/qmlcppengine.h>
#include <debugger/qml/qmlengine.h>
#include <debugger/stackhandler.h>
#include <debugger/stackframe.h>

#include <QMenu>
#include <QTextBlock>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QCheckBox>

static const char SCRIPT_CONSOLE[] = "ScriptConsole";
static const char SHOW_LOG[] = "showLog";
static const char SHOW_WARNING[] = "showWarning";
static const char SHOW_ERROR[] = "showError";

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// QmlJSScriptConsoleWidget
//
///////////////////////////////////////////////////////////////////////

QmlJSScriptConsoleWidget::QmlJSScriptConsoleWidget(QWidget *parent)
    : QWidget(parent)
{
    const int statusBarHeight = 25;
    const int spacing = 7;
    const int buttonWidth = 25;

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    QWidget *statusbarContainer = new QWidget;
    statusbarContainer->setFixedHeight(statusBarHeight);
    QHBoxLayout *hbox = new QHBoxLayout(statusbarContainer);
    hbox->setMargin(0);
    hbox->setSpacing(0);

    //Status Label
    m_statusLabel = new Utils::StatusLabel;
    hbox->addSpacing(spacing);
    hbox->addWidget(m_statusLabel);
    hbox->addWidget(new Utils::StyledSeparator);

    //Filters
    hbox->addSpacing(spacing);
    m_showLog = new QToolButton(this);
    m_showLog->setAutoRaise(true);
    m_showLog->setFixedWidth(buttonWidth);
    m_showLogAction = new Utils::SavedAction(this);
    m_showLogAction->setDefaultValue(true);
    m_showLogAction->setSettingsKey(_(SCRIPT_CONSOLE), _(SHOW_LOG));
    m_showLogAction->setText(tr("Log"));
    m_showLogAction->setCheckable(true);
    m_showLogAction->setIcon(QIcon(_(":/debugger/images/log.png")));
//    connect(m_showLogAction, SIGNAL(toggled(bool)), this, SLOT(setDebugLevel()));
    m_showLog->setDefaultAction(m_showLogAction);

    m_showWarning = new QToolButton(this);
    m_showWarning->setAutoRaise(true);
    m_showWarning->setFixedWidth(buttonWidth);
    m_showWarningAction = new Utils::SavedAction(this);
    m_showWarningAction->setDefaultValue(true);
    m_showWarningAction->setSettingsKey(_(SCRIPT_CONSOLE), _(SHOW_WARNING));
    m_showWarningAction->setText(tr("Warning"));
    m_showWarningAction->setCheckable(true);
    m_showWarningAction->setIcon(QIcon(_(":/debugger/images/warning.png")));
//    connect(m_showWarningAction, SIGNAL(toggled(bool)), this, SLOT(setDebugLevel()));
    m_showWarning->setDefaultAction(m_showWarningAction);

    m_showError = new QToolButton(this);
    m_showError->setAutoRaise(true);
    m_showError->setFixedWidth(buttonWidth);
    m_showErrorAction = new Utils::SavedAction(this);
    m_showErrorAction->setDefaultValue(true);
    m_showErrorAction->setSettingsKey(_(SCRIPT_CONSOLE), _(SHOW_ERROR));
    m_showErrorAction->setText(tr("Error"));
    m_showErrorAction->setCheckable(true);
    m_showErrorAction->setIcon(QIcon(_(":/debugger/images/error.png")));
//    connect(m_showErrorAction, SIGNAL(toggled(bool)), this, SLOT(setDebugLevel()));
    m_showError->setDefaultAction(m_showErrorAction);

    hbox->addWidget(m_showLog);
    hbox->addSpacing(spacing);
    hbox->addWidget(m_showWarning);
    hbox->addSpacing(spacing);
    hbox->addWidget(m_showError);

    //Clear Button
    QToolButton *clearButton = new QToolButton;
    clearButton->setAutoRaise(true);
    clearButton->setFixedWidth(buttonWidth);
    QAction *clearAction = new QAction(tr("Clear Console"), this);
    clearAction->setIcon(QIcon(_(Core::Constants::ICON_CLEAN_PANE)));
    clearButton->setDefaultAction(clearAction);

    hbox->addSpacing(spacing);
    hbox->addWidget(clearButton);
    hbox->addSpacing(spacing);

    m_consoleView = new ConsoleTreeView(this);

    m_model = new ConsoleItemModel(this);
    m_model->clear();
    connect(clearAction, SIGNAL(triggered()), m_model, SLOT(clear()));
    m_consoleView->setModel(m_model);
    connect(m_model,
            SIGNAL(editableRowAppended(QModelIndex,QItemSelectionModel::SelectionFlags)),
            m_consoleView->selectionModel(),
            SLOT(setCurrentIndex(QModelIndex,QItemSelectionModel::SelectionFlags)));

    m_consoleBackend = new QmlJSConsoleBackend(this);
    connect(m_consoleBackend, SIGNAL(evaluateExpression(QString)),
            this, SIGNAL(evaluateExpression(QString)));
    connect(m_consoleBackend, SIGNAL(message(ConsoleItemModel::ItemType,QString)),
            this, SLOT(appendOutput(ConsoleItemModel::ItemType,QString)));

    m_itemDelegate = new ConsoleItemDelegate(this);
    connect(m_itemDelegate, SIGNAL(appendEditableRow()),
            m_model, SLOT(appendEditableRow()));
    m_itemDelegate->setConsoleBackend(m_consoleBackend);
    m_consoleView->setItemDelegate(m_itemDelegate);

    vbox->addWidget(statusbarContainer);
    vbox->addWidget(m_consoleView);

    readSettings();
    connect(Core::ICore::instance(),
            SIGNAL(saveSettingsRequested()), SLOT(writeSettings()));
}

QmlJSScriptConsoleWidget::~QmlJSScriptConsoleWidget()
{
    writeSettings();
}

void QmlJSScriptConsoleWidget::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    m_showLogAction->readSettings(settings);
    m_showWarningAction->readSettings(settings);
    m_showErrorAction->readSettings(settings);
}

void QmlJSScriptConsoleWidget::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    m_showLogAction->writeSettings(settings);
    m_showWarningAction->writeSettings(settings);
    m_showErrorAction->writeSettings(settings);
}

void QmlJSScriptConsoleWidget::setEngine(DebuggerEngine *engine)
{
    QmlEngine *qmlEngine = m_consoleBackend->engine();
    if (qmlEngine) {
        disconnect(qmlEngine, SIGNAL(stateChanged(Debugger::DebuggerState)),
                   this, SLOT(onEngineStateChanged(Debugger::DebuggerState)));
        disconnect(qmlEngine->stackHandler(), SIGNAL(currentIndexChanged()),
                   this, SLOT(onSelectionChanged()));
        disconnect(qmlEngine->adapter(), SIGNAL(selectionChanged()),
                   this, SLOT(onSelectionChanged()));
        disconnect(qmlEngine->adapter()->messageClient(),
                   SIGNAL(message(QtMsgType,QString)),
                this, SLOT(appendMessage(QtMsgType,QString)));
        qmlEngine = 0;
    }

    qmlEngine = qobject_cast<QmlEngine *>(engine);
    QmlCppEngine *qmlCppEngine = qobject_cast<QmlCppEngine *>(engine);
    if (qmlCppEngine)
        qmlEngine = qobject_cast<QmlEngine *>(qmlCppEngine->qmlEngine());

    //Supports only QML Engine
    if (qmlEngine) {
        connect(qmlEngine, SIGNAL(stateChanged(Debugger::DebuggerState)),
                this, SLOT(onEngineStateChanged(Debugger::DebuggerState)));
        connect(qmlEngine->stackHandler(), SIGNAL(currentIndexChanged()),
                this, SLOT(onSelectionChanged()));
        connect(qmlEngine->adapter(), SIGNAL(selectionChanged()),
                this, SLOT(onSelectionChanged()));
        connect(qmlEngine->adapter()->messageClient(),
                SIGNAL(message(QtMsgType,QString)),
                this, SLOT(appendMessage(QtMsgType,QString)));
        onEngineStateChanged(qmlEngine->state());
    }
    m_consoleBackend->setEngine(qmlEngine);
}

void QmlJSScriptConsoleWidget::appendResult(const QString &result)
{
    m_model->appendItem(ConsoleItemModel::UndefinedType, result);
}

void QmlJSScriptConsoleWidget::onEngineStateChanged(Debugger::DebuggerState state)
{
    if (state == InferiorRunOk || state == InferiorStopOk) {
        setEnabled(true);
        m_consoleBackend->setInferiorStopped(state == InferiorStopOk);
    } else {
        setEnabled(false);
    }
}

void QmlJSScriptConsoleWidget::onSelectionChanged()
{
    QmlEngine *qmlEngine = m_consoleBackend->engine();
    if (qmlEngine && qmlEngine->adapter()) {
        const QString context = m_consoleBackend->inferiorStopped() ?
                    qmlEngine->stackHandler()->currentFrame().function :
                    qmlEngine->adapter()->currentSelectedDisplayName();

        m_consoleBackend->setIsValidContext(!context.isEmpty());
        m_statusLabel->showStatusMessage(tr("Context: ").append(context), 0);
    }
}

void QmlJSScriptConsoleWidget::appendOutput(ConsoleItemModel::ItemType itemType,
                                            const QString &message)
{
    if (itemType == ConsoleItemModel::UndefinedType)
        return m_model->appendItem(itemType, message);

    QtMsgType type;
    switch (itemType) {
    case ConsoleItemModel::LogType:
        type = QtDebugMsg;
        break;
    case ConsoleItemModel::WarningType:
        type = QtWarningMsg;
        break;
    case ConsoleItemModel::ErrorType:
        type = QtCriticalMsg;
        break;
    default:
        type = QtDebugMsg;
        break;
    }
    appendMessage(type, message);
}

void QmlJSScriptConsoleWidget::appendMessage(QtMsgType type, const QString &message)
{
    ConsoleItemModel::ItemType itemType;
    switch (type) {
    case QtDebugMsg:
        if (!m_showLogAction->isChecked())
            return;
        itemType = ConsoleItemModel::LogType;
        break;
    case QtWarningMsg:
        if (!m_showWarningAction->isChecked())
            return;
        itemType = ConsoleItemModel::WarningType;
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        if (!m_showErrorAction->isChecked())
            return;
        itemType = ConsoleItemModel::ErrorType;
        break;
    default:
        //this cannot happen as type has to
        //be one of the above
        //return since itemType is not known
        return;
    }
    m_model->appendItem(itemType, message);
}

} //Internal
} //Debugger
