/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "minimizableinfobars.h"

#include "qtcassert.h"
#include "qtcsettings.h"
#include "utilsicons.h"

#include <QToolButton>

const char SETTINGS_PREFIX[] = "ShowInfoBarFor";
const bool kShowInInfoBarDefault = true;

namespace Utils {

MinimizableInfoBars::MinimizableInfoBars(InfoBar &infoBar)
    : m_infoBar(infoBar)
{
}

void MinimizableInfoBars::setPossibleInfoBarEntries(const QList<Utils::InfoBarEntry> &entries)
{
    QTC_CHECK(m_actions.isEmpty());
    m_infoEntries.clear();
    m_isInfoVisible.clear();
    for (const Utils::InfoBarEntry &entry : entries) {
        m_infoEntries.insert(entry.id(), entry);
        m_isInfoVisible.insert(entry.id(), false);
    }
    createActions();
}

void MinimizableInfoBars::setSettingsGroup(const QString &settingsGroup)
{
    m_settingsGroup = settingsGroup;
}

void MinimizableInfoBars::createActions()
{
    QTC_CHECK(m_actions.isEmpty());
    for (const Utils::InfoBarEntry &entry : qAsConst(m_infoEntries)) {
        const Id id = entry.id();
        auto action = new QAction(this);
        action->setToolTip(entry.text());
        action->setIcon(Icons::WARNING_TOOLBAR.pixmap());
        connect(action, &QAction::triggered, this, [this, id]() {
            setShowInInfoBar(id, true);
            updateInfo(id);
        });
        action->setVisible(!showInInfoBar(id));
        m_actions.insert(id, action);
    }
}

QString MinimizableInfoBars::settingsKey(const Id &id) const
{
    QTC_CHECK(!m_settingsGroup.isEmpty());
    return m_settingsGroup + '/' + SETTINGS_PREFIX + id.toString();
}

void MinimizableInfoBars::createShowInfoBarActions(const ActionCreator &actionCreator) const
{
    QTC_ASSERT(actionCreator, return );

    for (QAction *action : m_actions) {
        auto *button = new QToolButton();
        button->setDefaultAction(action);
        QAction *toolbarAction = actionCreator(button);
        connect(action, &QAction::changed, toolbarAction, [action, toolbarAction] {
            toolbarAction->setVisible(action->isVisible());
        });
        toolbarAction->setVisible(action->isVisible());
    }
}

void MinimizableInfoBars::setInfoVisible(const Id &id, bool visible)
{
    QTC_CHECK(m_isInfoVisible.contains(id));
    m_isInfoVisible.insert(id, visible);
    updateInfo(id);
}

void MinimizableInfoBars::updateInfo(const Id &id)
{
    m_infoBar.removeInfo(id);

    bool show = false;
    if (m_isInfoVisible.value(id)) {
        if (showInInfoBar(id))
            showInfoBar(id);
        else
            show = true;
    }

    QAction *action = m_actions.value(id);
    if (QTC_GUARD(action))
        action->setVisible(show);
}

void MinimizableInfoBars::showInfoBar(const Id &id)
{
    const InfoBarEntry entry = m_infoEntries.value(id);
    InfoBarEntry info(entry);
    info.removeCancelButton();
    // The minimizer() might delete the "Minimize" button immediately and as
    // result invalid reads will happen in QToolButton::mouseReleaseEvent().
    // Avoid this by running the minimizer in the next event loop iteration.
    info.addCustomButton(MinimizableInfoBars::tr("Minimize"), [this, id] {
        QMetaObject::invokeMethod(
            this,
            [id, this] {
                setShowInInfoBar(id, false);
                updateInfo(id);
            },
            Qt::QueuedConnection);
    });
    m_infoBar.addInfo(info);
}

bool MinimizableInfoBars::showInInfoBar(const Id &id) const
{
    return InfoBar::settings()->value(settingsKey(id), kShowInInfoBarDefault).toBool();
}

void MinimizableInfoBars::setShowInInfoBar(const Id &id, bool show)
{
    QtcSettings::setValueWithDefault(InfoBar::settings(),
                                     settingsKey(id),
                                     show,
                                     kShowInInfoBarDefault);
}

} // namespace Utils
