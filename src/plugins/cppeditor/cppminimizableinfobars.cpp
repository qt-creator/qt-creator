/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppminimizableinfobars.h"

#include <QToolButton>

#include <coreplugin/icore.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

const char NO_PROJECT_CONFIGURATION[] = "CppEditor.NoProjectConfiguration";
const char CPPEDITOR_SHOW_INFO_BAR_FOR_FOR_NO_PROJECT[] = "ShowInfoBarForNoProject";
const bool kShowInInfoBarDefault = true;

using namespace Core;
using namespace Utils;

namespace CppEditor {
namespace Internal {

MinimizableInfoBars::MinimizableInfoBars(InfoBar &infoBar)
    : m_infoBar(infoBar)
{
    createActions();
}

void MinimizableInfoBars::setSettingsGroup(const QString &settingsGroup)
{
    m_settingsGroup = settingsGroup;
}

void MinimizableInfoBars::createActions()
{
    auto action = new QAction(this);
    action->setToolTip(tr("File is not part of any project."));
    action->setIcon(Icons::WARNING_TOOLBAR.pixmap());
    connect(action, &QAction::triggered, this, [this]() { setShowNoProjectInfoBar(true); });
    action->setVisible(!showNoProjectInfoBar());
    m_actions.insert(NO_PROJECT_CONFIGURATION, action);
}

QString MinimizableInfoBars::settingsKey(const QString &id) const
{
    QTC_CHECK(!m_settingsGroup.isEmpty());
    return m_settingsGroup + '/' + id;
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

void MinimizableInfoBars::processHasProjectPart(bool hasProjectPart)
{
    m_hasProjectPart = hasProjectPart;
    updateNoProjectConfiguration();
}

void MinimizableInfoBars::updateNoProjectConfiguration()
{
    const Id id(NO_PROJECT_CONFIGURATION);
    m_infoBar.removeInfo(id);

    bool show = false;
    if (!m_hasProjectPart) {
        if (showNoProjectInfoBar())
            addNoProjectConfigurationEntry(id);
        else
            show = true;
    }

    QAction *action = m_actions.value(id);
    if (QTC_GUARD(action))
        action->setVisible(show);
}

static InfoBarEntry createMinimizableInfo(const Id &id,
                                          const QString &text,
                                          QObject *guard,
                                          std::function<void()> minimizer)
{
    QTC_CHECK(minimizer);

    InfoBarEntry info(id, text);
    info.removeCancelButton();
    // The minimizer() might delete the "Minimize" button immediately and as
    // result invalid reads will happen in QToolButton::mouseReleaseEvent().
    // Avoid this by running the minimizer in the next event loop iteration.
    info.addCustomButton(MinimizableInfoBars::tr("Minimize"), [guard, minimizer] {
        QMetaObject::invokeMethod(
            guard, [minimizer] { minimizer(); }, Qt::QueuedConnection);
    });

    return info;
}

void MinimizableInfoBars::addNoProjectConfigurationEntry(const Id &id)
{
    const QString text = tr("<b>Warning</b>: This file is not part of any project. "
                            "The code model might have issues parsing this file properly.");

    m_infoBar.addInfo(
        createMinimizableInfo(id, text, this, [this]() { setShowNoProjectInfoBar(false); }));
}

bool MinimizableInfoBars::showNoProjectInfoBar() const
{
    return ICore::settings()
        ->value(settingsKey(CPPEDITOR_SHOW_INFO_BAR_FOR_FOR_NO_PROJECT), kShowInInfoBarDefault)
        .toBool();
}

void MinimizableInfoBars::setShowNoProjectInfoBar(bool show)
{
    ICore::settings()->setValueWithDefault(settingsKey(CPPEDITOR_SHOW_INFO_BAR_FOR_FOR_NO_PROJECT),
                                           show,
                                           kShowInInfoBarDefault);
    updateNoProjectConfiguration();
}

} // namespace Internal
} // namespace CppEditor
