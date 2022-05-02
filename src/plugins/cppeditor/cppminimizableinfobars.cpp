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

#include "cpptoolssettings.h"

#include <QToolButton>

#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

const char NO_PROJECT_CONFIGURATION[] = "CppEditor.NoProjectConfiguration";

using namespace Utils;

namespace CppEditor {
namespace Internal {

static CppToolsSettings *settings() { return CppToolsSettings::instance(); }

MinimizableInfoBars::MinimizableInfoBars(InfoBar &infoBar, QObject *parent)
    : QObject(parent)
    , m_infoBar(infoBar)
{
    connect(settings(), &CppToolsSettings::showNoProjectInfoBarChanged,
            this, &MinimizableInfoBars::updateNoProjectConfiguration);
    createActions();
}

void MinimizableInfoBars::createActions()
{
    auto action = new QAction(this);
    action->setToolTip(tr("File is not part of any project."));
    action->setIcon(Icons::WARNING_TOOLBAR.pixmap());
    connect(action, &QAction::triggered, []() { settings()->setShowNoProjectInfoBar(true); });
    action->setVisible(!settings()->showNoProjectInfoBar());
    m_actions.insert(NO_PROJECT_CONFIGURATION, action);
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
        if (settings()->showNoProjectInfoBar())
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
                                          std::function<void()> minimizer)
{
    QTC_CHECK(minimizer);

    InfoBarEntry info(id, text);
    info.removeCancelButton();
    // The minimizer() might delete the "Minimize" button immediately and as
    // result invalid reads will happen in QToolButton::mouseReleaseEvent().
    // Avoid this by running the minimizer in the next event loop iteration.
    info.addCustomButton(MinimizableInfoBars::tr("Minimize"), [minimizer] {
        QMetaObject::invokeMethod(settings(), [minimizer] { minimizer(); }, Qt::QueuedConnection);
    });

    return info;
}

void MinimizableInfoBars::addNoProjectConfigurationEntry(const Id &id)
{
    const QString text = tr("<b>Warning</b>: This file is not part of any project. "
                            "The code model might have issues parsing this file properly.");

    m_infoBar.addInfo(createMinimizableInfo(id, text, []() {
        settings()->setShowNoProjectInfoBar(false);
    }));
}

} // namespace Internal
} // namespace CppEditor
