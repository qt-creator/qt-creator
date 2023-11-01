// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "minimizableinfobars.h"

#include "qtcassert.h"
#include "qtcsettings.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QAction>
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

void MinimizableInfoBars::setSettingsGroup(const Key &settingsGroup)
{
    m_settingsGroup = settingsGroup;
}

void MinimizableInfoBars::createActions()
{
    QTC_CHECK(m_actions.isEmpty());
    for (const Utils::InfoBarEntry &entry : std::as_const(m_infoEntries)) {
        const Id id = entry.id();
        auto action = new QAction(this);
        action->setToolTip(entry.text());
        action->setIcon(Icons::WARNING_TOOLBAR.pixmap());
        connect(action, &QAction::triggered, this, [this, id] {
            setShowInInfoBar(id, true);
            updateInfo(id);
        });
        action->setVisible(!showInInfoBar(id));
        m_actions.insert(id, action);
    }
}

Key MinimizableInfoBars::settingsKey(const Id &id) const
{
    QTC_CHECK(!m_settingsGroup.isEmpty());
    return m_settingsGroup + '/' + SETTINGS_PREFIX + id.name();
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
    info.addCustomButton(Tr::tr("Minimize"), [this, id] {
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
    InfoBar::settings()->setValueWithDefault(settingsKey(id), show, kShowInInfoBarDefault);
}

} // namespace Utils
