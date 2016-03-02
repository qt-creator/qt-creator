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

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QComboBox>
#include <QDockWidget>
#include <QStackedWidget>

using namespace Analyzer;
using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

MainWindowBase::MainWindowBase()
{
    m_controlsStackWidget = new QStackedWidget;
    m_statusLabel = new Utils::StatusLabel;
    m_toolBox = new QComboBox;

    setDockNestingEnabled(true);
    setDockActionsVisible(false);
    setDocumentMode(true);

    connect(this, &FancyMainWindow::resetLayout,
            this, &MainWindowBase::resetCurrentPerspective);
}

MainWindowBase::~MainWindowBase()
{
    // as we have to setParent(0) on dock widget that are not selected,
    // we keep track of all and make sure we don't leak any
    foreach (const DockPtr &ptr, m_dockWidgets) {
        if (ptr)
            delete ptr.data();
    }
}

void MainWindowBase::registerPerspective(Id perspectiveId, const Perspective &perspective)
{
    m_perspectiveForPerspectiveId.insert(perspectiveId, perspective);
}

void MainWindowBase::registerToolbar(Id perspectiveId, QWidget *widget)
{
    m_toolbarForPerspectiveId.insert(perspectiveId, widget);
    m_controlsStackWidget->addWidget(widget);
}

void MainWindowBase::showStatusMessage(const QString &message, int timeoutMS)
{
    m_statusLabel->showStatusMessage(message, timeoutMS);
}

void MainWindowBase::resetCurrentPerspective()
{
    loadPerspectiveHelper(m_currentPerspectiveId, false);
}

void MainWindowBase::restorePerspective(Id perspectiveId)
{
    loadPerspectiveHelper(perspectiveId, true);
}

void MainWindowBase::loadPerspectiveHelper(Id perspectiveId, bool fromStoredSettings)
{
    QTC_ASSERT(perspectiveId.isValid(), return);

    // Clean up old perspective.
    closeCurrentPerspective();

    m_currentPerspectiveId = perspectiveId;

    QTC_ASSERT(m_perspectiveForPerspectiveId.contains(perspectiveId), return);
    const auto splits = m_perspectiveForPerspectiveId.value(perspectiveId).splits();
    for (const Perspective::Split &split : splits) {
        QDockWidget *dock = m_dockForDockId.value(split.dockId);
        QTC_ASSERT(dock, continue);
        addDockWidget(split.area, dock);
        QDockWidget *existing = m_dockForDockId.value(split.existing);
        if (!existing && split.area == Qt::BottomDockWidgetArea)
            existing = toolBarDockWidget();
        if (existing) {
            switch (split.splitType) {
            case Perspective::AddToTab:
                tabifyDockWidget(existing, dock);
                break;
            case Perspective::SplitHorizontal:
                splitDockWidget(existing, dock, Qt::Horizontal);
                break;
            case Perspective::SplitVertical:
                splitDockWidget(existing, dock, Qt::Vertical);
                break;
            }
        }
        if (!split.visibleByDefault)
            dock->hide();
        else
            dock->show();
    }

    if (fromStoredSettings) {
        QSettings *settings = ICore::settings();
        settings->beginGroup(perspectiveId.toString());
        if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
            restoreSettings(settings);
        settings->endGroup();
    }

    QTC_CHECK(m_toolbarForPerspectiveId.contains(perspectiveId));
    m_controlsStackWidget->setCurrentWidget(m_toolbarForPerspectiveId.value(perspectiveId));
}

void MainWindowBase::closeCurrentPerspective()
{
    if (!m_currentPerspectiveId.isValid())
        return;

    saveCurrentPerspective();
    foreach (QDockWidget *dockWidget, m_dockForDockId) {
        QTC_ASSERT(dockWidget, continue);
        removeDockWidget(dockWidget);
        dockWidget->hide();
        // Prevent saveState storing the data of the wrong children.
        dockWidget->setParent(0);
    }
}

void MainWindowBase::saveCurrentPerspective()
{
    if (!m_currentPerspectiveId.isValid())
        return;
    QSettings *settings = ICore::settings();
    settings->beginGroup(m_currentPerspectiveId.toString());
    saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(m_lastSettingsName, m_currentPerspectiveId.toString());
}

QDockWidget *MainWindowBase::registerDockWidget(Id dockId, QWidget *widget)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return 0);
    QDockWidget *dockWidget = addDockForWidget(widget);
    m_dockWidgets.append(MainWindowBase::DockPtr(dockWidget));
    m_dockForDockId[dockId] = dockWidget;
    return dockWidget;
}

Core::Id MainWindowBase::currentPerspectiveId() const
{
    return m_currentPerspectiveId;
}

QString MainWindowBase::lastSettingsName() const
{
    return m_lastSettingsName;
}

void MainWindowBase::setLastSettingsName(const QString &lastSettingsName)
{
    m_lastSettingsName = lastSettingsName;
}

} // Internal
} // Debugger
