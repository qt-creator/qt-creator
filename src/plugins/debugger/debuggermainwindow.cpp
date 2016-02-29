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
    m_statusLabelsStackWidget= new QStackedWidget;
    m_toolBox = new QComboBox;

    setDockNestingEnabled(true);
    setDockActionsVisible(false);
    setDocumentMode(true);
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

void MainWindowBase::addPerspective(const Perspective &perspective)
{
    m_perspectives.append(perspective);
}

void MainWindowBase::showStatusMessage(Id perspective, const QString &message, int timeoutMS)
{
    StatusLabel *statusLabel = m_statusLabelForPerspective.value(perspective);
    QTC_ASSERT(statusLabel, return);
    statusLabel->showStatusMessage(message, timeoutMS);
}

void MainWindowBase::restorePerspective(Id perspectiveId,
                                        std::function<QWidget *()> creator,
                                        bool fromStoredSettings)
{
    if (!perspectiveId.isValid())
        return;

    if (!m_defaultSettings.contains(perspectiveId) && creator) {
        QWidget *widget = creator();
        QTC_CHECK(widget);
        m_defaultSettings.insert(perspectiveId);
        QTC_CHECK(!m_controlsWidgetForPerspective.contains(perspectiveId));
        m_controlsWidgetForPerspective[perspectiveId] = widget;
        m_controlsStackWidget->addWidget(widget);
        StatusLabel * const toolStatusLabel = new StatusLabel;
        m_statusLabelForPerspective[perspectiveId] = toolStatusLabel;
        m_statusLabelsStackWidget->addWidget(toolStatusLabel);
    }

    const Perspective *perspective = findPerspective(perspectiveId);
    QTC_ASSERT(perspective, return);

    foreach (const Perspective::Split &split, perspective->splits()) {
        QDockWidget *dock = m_dockForDockId.value(split.dockId);
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
    }

    if (fromStoredSettings) {
        QSettings *settings = ICore::settings();
        settings->beginGroup(m_settingsName + perspectiveId.toString());
        if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
            restoreSettings(settings);
        settings->endGroup();
    }

    QTC_CHECK(m_controlsWidgetForPerspective.contains(perspectiveId));
    m_controlsStackWidget->setCurrentWidget(m_controlsWidgetForPerspective.value(perspectiveId));
    m_statusLabelsStackWidget->setCurrentWidget(m_statusLabelForPerspective.value(perspectiveId));
}

void MainWindowBase::closePerspective(Id perspectiveId)
{
    if (!perspectiveId.isValid())
        return;
    savePerspective(perspectiveId);
    const Perspective *perspective = findPerspective(perspectiveId);
    QTC_ASSERT(perspective, return);
    foreach (Id dockId, perspective->docks()) {
        QDockWidget *dockWidget = m_dockForDockId.value(dockId);
        QTC_ASSERT(dockWidget, continue);
        removeDockWidget(dockWidget);
        dockWidget->hide();
        // Prevent saveState storing the data of the wrong children.
        dockWidget->setParent(0);
    }
}

const Perspective *MainWindowBase::findPerspective(Id perspectiveId) const
{
    foreach (const Perspective &perspective, m_perspectives)
        if (perspective.id() == perspectiveId)
            return &perspective;
    return 0;
}

void MainWindowBase::savePerspective(Id perspectiveId)
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(m_settingsName + perspectiveId.toString());
    saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(m_lastSettingsName, perspectiveId.toString());
}

QDockWidget *MainWindowBase::createDockWidget(QWidget *widget, Id dockId)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return 0);
    QDockWidget *dockWidget = addDockForWidget(widget);
    m_dockWidgets.append(MainWindowBase::DockPtr(dockWidget));
    m_dockForDockId[dockId] = dockWidget;
    return dockWidget;
}

QString MainWindowBase::settingsName() const
{
    return m_settingsName;
}

void MainWindowBase::setSettingsName(const QString &settingsName)
{
    m_settingsName = settingsName;
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
