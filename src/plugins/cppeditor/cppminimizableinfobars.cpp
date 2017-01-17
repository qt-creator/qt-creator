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

#include "cppeditorconstants.h"

#include <QTimer>
#include <QToolButton>

#include <coreplugin/id.h>
#include <coreplugin/infobar.h>

#include <cpptools/cpptoolssettings.h>

#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace CppTools;

namespace CppEditor {
namespace Internal {

static CppToolsSettings *settings() { return CppToolsSettings::instance(); }

MinimizableInfoBars::MinimizableInfoBars(InfoBar &infoBar, QObject *parent)
    : QObject(parent)
    , m_infoBar(infoBar)
{
    connect(settings(), &CppToolsSettings::showHeaderErrorInfoBarChanged,
            this, &MinimizableInfoBars::updateHeaderErrors);
    connect(settings(), &CppToolsSettings::showNoProjectInfoBarChanged,
            this, &MinimizableInfoBars::updateNoProjectConfiguration);
}

MinimizableInfoBars::Actions MinimizableInfoBars::createShowInfoBarActions(
        const ActionCreator &actionCreator)
{
    Actions result;
    QTC_ASSERT(actionCreator, return result);

    // No project configuration available
    auto *button = new QToolButton();
    button->setToolTip(tr("File is not part of any project."));
    button->setIcon(Utils::Icons::WARNING_TOOLBAR.pixmap());
    connect(button, &QAbstractButton::clicked, []() {
        settings()->setShowNoProjectInfoBar(true);
    });
    QAction *action = actionCreator(button);
    action->setVisible(!settings()->showNoProjectInfoBar());
    result.insert(Constants::NO_PROJECT_CONFIGURATION, action);

    // Errors in included files
    button = new QToolButton();
    button->setToolTip(tr("File contains errors in included files."));
    button->setIcon(Utils::Icons::WARNING_TOOLBAR.pixmap());
    connect(button, &QAbstractButton::clicked, []() {
        settings()->setShowHeaderErrorInfoBar(true);
    });
    action = actionCreator(button);
    action->setVisible(!settings()->showHeaderErrorInfoBar());
    result.insert(Constants::ERRORS_IN_HEADER_FILES, action);

    return result;
}

void MinimizableInfoBars::processHeaderDiagnostics(
        const DiagnosticWidgetCreator &diagnosticWidgetCreator)
{
    m_diagnosticWidgetCreator = diagnosticWidgetCreator;
    updateHeaderErrors();
}

void MinimizableInfoBars::processHasProjectPart(bool hasProjectPart)
{
    m_hasProjectPart = hasProjectPart;
    updateNoProjectConfiguration();
}

void MinimizableInfoBars::updateHeaderErrors()
{
    const Id id(Constants::ERRORS_IN_HEADER_FILES);
    m_infoBar.removeInfo(id);

    bool show = false;
    // Show the info entry only if there is a project configuration.
    if (m_hasProjectPart && m_diagnosticWidgetCreator) {
        if (settings()->showHeaderErrorInfoBar())
            addHeaderErrorEntry(id, m_diagnosticWidgetCreator);
        else
            show = true;
    }

    emit showAction(id, show);
}

void MinimizableInfoBars::updateNoProjectConfiguration()
{
    const Id id(Constants::NO_PROJECT_CONFIGURATION);
    m_infoBar.removeInfo(id);

    bool show = false;
    if (!m_hasProjectPart) {
        if (settings()->showNoProjectInfoBar())
            addNoProjectConfigurationEntry(id);
        else
            show = true;
    }

    emit showAction(id, show);
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
    info.setCustomButtonInfo(MinimizableInfoBars::tr("Minimize"), [=](){
        QTimer::singleShot(0, [=] {
            minimizer();
        });
    });

    return info;
}

void MinimizableInfoBars::addNoProjectConfigurationEntry(const Id &id)
{
    const QString text = tr("<b>Warning</b>: This file is not part of any project. "
                            "The code model might have issues to parse this file properly.");

    m_infoBar.addInfo(createMinimizableInfo(id, text, []() {
        settings()->setShowNoProjectInfoBar(false);
    }));
}

void MinimizableInfoBars::addHeaderErrorEntry(const Id &id,
        const DiagnosticWidgetCreator &diagnosticWidgetCreator)
{
    const QString text = tr("<b>Warning</b>: The code model could not parse an included file, "
                            "which might lead to slow or incorrect code completion and "
                            "highlighting, for example.");

    InfoBarEntry info = createMinimizableInfo(id, text, []() {
        settings()->setShowHeaderErrorInfoBar(false);
    });
    info.setDetailsWidgetCreator(diagnosticWidgetCreator);

    m_infoBar.addInfo(info);
}

} // namespace Internal
} // namespace CppEditor
