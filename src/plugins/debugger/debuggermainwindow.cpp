/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "debuggermainwindow.h"
#include "debuggerconstants.h"
#include "debuggerinternalconstants.h"
#include "analyzer/analyzericons.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorericons.h>

#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>

using namespace Debugger;
using namespace Core;

namespace Utils {

const char LAST_PERSPECTIVE_KEY[]   = "LastPerspective";

DebuggerMainWindow::DebuggerMainWindow()
{
    m_controlsStackWidget = new QStackedWidget;
    m_statusLabel = new Utils::StatusLabel;

    m_perspectiveChooser = new QComboBox;
    m_perspectiveChooser->setObjectName(QLatin1String("PerspectiveChooser"));
    connect(m_perspectiveChooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this](int item) { restorePerspective(m_perspectiveChooser->itemData(item).toByteArray()); });

    setDockNestingEnabled(true);
    setDockActionsVisible(false);
    setDocumentMode(true);

    connect(this, &FancyMainWindow::resetLayout,
            this, &DebuggerMainWindow::resetCurrentPerspective);

    auto dock = new QDockWidget(tr("Toolbar"));
    dock->setObjectName(QLatin1String("Toolbar"));
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock)); // hide title bar
    dock->setProperty("managed_dockwidget", QLatin1String("true"));

    addDockWidget(Qt::BottomDockWidgetArea, dock);
    setToolBarDockWidget(dock);
}

DebuggerMainWindow::~DebuggerMainWindow()
{
    // as we have to setParent(0) on dock widget that are not selected,
    // we keep track of all and make sure we don't leak any
    foreach (const DockPtr &ptr, m_dockWidgets) {
        if (ptr)
            delete ptr.data();
    }
}

void DebuggerMainWindow::registerPerspective(const QByteArray &perspectiveId, const Perspective &perspective)
{
    m_perspectiveForPerspectiveId.insert(perspectiveId, perspective);
    m_perspectiveChooser->addItem(perspective.name(), perspectiveId);
    // adjust width if necessary
    const int oldWidth = m_perspectiveChooser->width();
    const int contentWidth = m_perspectiveChooser->fontMetrics().width(perspective.name());
    QStyleOptionComboBox option;
    option.initFrom(m_perspectiveChooser);
    const QSize sz(contentWidth, 1);
    const int width = m_perspectiveChooser->style()->sizeFromContents(
                QStyle::CT_ComboBox, &option, sz).width();
    if (width > oldWidth)
        m_perspectiveChooser->setFixedWidth(width);
}

void DebuggerMainWindow::registerToolbar(const QByteArray &perspectiveId, QWidget *widget)
{
    m_toolbarForPerspectiveId.insert(perspectiveId, widget);
    m_controlsStackWidget->addWidget(widget);
}

void DebuggerMainWindow::showStatusMessage(const QString &message, int timeoutMS)
{
    m_statusLabel->showStatusMessage(message, timeoutMS);
}

QDockWidget *DebuggerMainWindow::dockWidget(const QByteArray &dockId) const
{
   return m_dockForDockId.value(dockId);
}

void DebuggerMainWindow::resetCurrentPerspective()
{
    loadPerspectiveHelper(m_currentPerspectiveId, false);
}

void DebuggerMainWindow::restorePerspective(const QByteArray &perspectiveId)
{
    loadPerspectiveHelper(perspectiveId, true);

    int index = m_perspectiveChooser->findData(perspectiveId);
    if (index == -1)
        index = m_perspectiveChooser->findData(m_currentPerspectiveId);
    if (index != -1)
        m_perspectiveChooser->setCurrentIndex(index);
}

void DebuggerMainWindow::finalizeSetup()
{
    auto viewButton = new QToolButton;
    viewButton->setText(tr("Views"));

    auto toolbar = new Utils::StyledBar;
    toolbar->setProperty("topBorder", true);
    auto hbox = new QHBoxLayout(toolbar);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(m_perspectiveChooser);
    hbox->addWidget(m_controlsStackWidget);
    hbox->addWidget(m_statusLabel);
    hbox->addWidget(new Utils::StyledSeparator);
    hbox->addStretch();
    hbox->addWidget(viewButton);

    connect(viewButton, &QAbstractButton::clicked, [this, viewButton] {
        QMenu menu;
        addDockActionsToMenu(&menu);
        menu.exec(viewButton->mapToGlobal(QPoint()));
    });

    toolBarDockWidget()->setWidget(toolbar);

    Context debugcontext(Debugger::Constants::C_DEBUGMODE);

    ActionContainer *viewsMenu = ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS);
    Command *cmd = ActionManager::registerAction(menuSeparator1(),
        "Debugger.Views.Separator1", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(autoHideTitleBarsAction(),
        "Debugger.Views.AutoHideTitleBars", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(menuSeparator2(),
        "Debugger.Views.Separator2", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(resetLayoutAction(),
        "Debugger.Views.ResetSimple", debugcontext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    addDockActionsToMenu(viewsMenu->menu());
}

void DebuggerMainWindow::loadPerspectiveHelper(const QByteArray &perspectiveId, bool fromStoredSettings)
{
    // Clean up old perspective.
    if (!m_currentPerspectiveId.isEmpty()) {
        saveCurrentPerspective();
        foreach (QDockWidget *dockWidget, m_dockForDockId) {
            QTC_ASSERT(dockWidget, continue);
            removeDockWidget(dockWidget);
            dockWidget->hide();
            // Prevent saveState storing the data of the wrong children.
            dockWidget->setParent(0);
        }

        ICore::removeAdditionalContext(Context(Id::fromName(m_currentPerspectiveId)));
    }

    m_currentPerspectiveId = perspectiveId;

    if (m_currentPerspectiveId.isEmpty()) {
        const QSettings *settings = ICore::settings();
        m_currentPerspectiveId = settings->value(QLatin1String(LAST_PERSPECTIVE_KEY)).toByteArray();
        if (m_currentPerspectiveId.isEmpty())
            m_currentPerspectiveId = Debugger::Constants::CppPerspectiveId;
    }

    ICore::addAdditionalContext(Context(Id::fromName(m_currentPerspectiveId)));

    QTC_ASSERT(m_perspectiveForPerspectiveId.contains(m_currentPerspectiveId), return);
    const auto operations = m_perspectiveForPerspectiveId.value(m_currentPerspectiveId).operations();
    for (const Perspective::Operation &operation : operations) {
        QDockWidget *dock = m_dockForDockId.value(operation.dockId);
        if (!dock) {
            QTC_CHECK(!operation.widget->objectName().isEmpty());
            dock = registerDockWidget(operation.dockId, operation.widget);

            QAction *toggleViewAction = dock->toggleViewAction();
            toggleViewAction->setText(dock->windowTitle());

            Command *cmd = ActionManager::registerAction(toggleViewAction,
                Id("Dock.").withSuffix(dock->objectName()),
                Context(Id::fromName(m_currentPerspectiveId)));
            cmd->setAttribute(Command::CA_Hide);

            ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS)->addAction(cmd);
        }
        if (operation.operationType == Perspective::Raise) {
            dock->raise();
            continue;
        }
        addDockWidget(operation.area, dock);
        QDockWidget *anchor = m_dockForDockId.value(operation.anchorDockId);
        if (!anchor && operation.area == Qt::BottomDockWidgetArea)
            anchor = toolBarDockWidget();
        if (anchor) {
            switch (operation.operationType) {
            case Perspective::AddToTab:
                tabifyDockWidget(anchor, dock);
                break;
            case Perspective::SplitHorizontal:
                splitDockWidget(anchor, dock, Qt::Horizontal);
                break;
            case Perspective::SplitVertical:
                splitDockWidget(anchor, dock, Qt::Vertical);
                break;
            default:
                break;
            }
        }
        if (!operation.visibleByDefault)
            dock->hide();
        else
            dock->show();
    }

    if (fromStoredSettings) {
        QSettings *settings = ICore::settings();
        settings->beginGroup(QString::fromLatin1(m_currentPerspectiveId));
        if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
            restoreSettings(settings);
        settings->endGroup();
    }

    QTC_CHECK(m_toolbarForPerspectiveId.contains(m_currentPerspectiveId));
    m_controlsStackWidget->setCurrentWidget(m_toolbarForPerspectiveId.value(m_currentPerspectiveId));
    m_statusLabel->clear();
}

void DebuggerMainWindow::saveCurrentPerspective()
{
    if (m_currentPerspectiveId.isEmpty())
        return;
    QSettings *settings = ICore::settings();
    settings->beginGroup(QString::fromLatin1(m_currentPerspectiveId));
    saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(QLatin1String(LAST_PERSPECTIVE_KEY), m_currentPerspectiveId);
}

QDockWidget *DebuggerMainWindow::registerDockWidget(const QByteArray &dockId, QWidget *widget)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return 0);
    QDockWidget *dockWidget = addDockForWidget(widget);
    dockWidget->setParent(0);
    m_dockWidgets.append(DebuggerMainWindow::DockPtr(dockWidget));
    m_dockForDockId[dockId] = dockWidget;
    return dockWidget;
}

QString Perspective::name() const
{
    return m_name;
}

void Perspective::setName(const QString &name)
{
    m_name = name;
}

QList<QWidget *> ToolbarDescription::widgets() const
{
    return m_widgets;
}

void ToolbarDescription::addAction(QAction *action)
{
    auto button = new QToolButton;
    button->setDefaultAction(action);
    m_widgets.append(button);
}

void ToolbarDescription::addWidget(QWidget *widget)
{
    m_widgets.append(widget);
}

Perspective::Operation::Operation(const QByteArray &dockId, QWidget *widget, const QByteArray &anchorDockId,
                                  Perspective::OperationType splitType, bool visibleByDefault,
                                  Qt::DockWidgetArea area)
    : dockId(dockId), widget(widget), anchorDockId(anchorDockId),
      operationType(splitType), visibleByDefault(visibleByDefault), area(area)
{}

Perspective::Perspective(const QString &name, const QVector<Operation> &splits)
    : m_name(name), m_operations(splits)
{
    for (const Operation &split : splits)
        m_docks.append(split.dockId);
}

void Perspective::addOperation(const Operation &operation)
{
    m_docks.append(operation.dockId);
    m_operations.append(operation);
}

} // Utils
