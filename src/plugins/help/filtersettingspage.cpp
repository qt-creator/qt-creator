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

#include "filtersettingspage.h"
#include "helpconstants.h"

#include <QtCore/QVersionNumber>
#include <QtHelp/QHelpFilterEngine>
#include <QtHelp/QHelpFilterSettingsWidget>
#include "localhelpmanager.h"

using namespace Help::Internal;

FilterSettingsPage::FilterSettingsPage()
{
    setId("D.Filters");
    setDisplayName(tr("Filters"));
    setCategory(Help::Constants::HELP_CATEGORY);
}

QWidget *FilterSettingsPage::widget()
{
    if (!m_widget) {
        LocalHelpManager::setupGuiHelpEngine();
        m_widget = new QHelpFilterSettingsWidget(nullptr);
        m_widget->readSettings(LocalHelpManager::filterEngine());

        connect(Core::HelpManager::Signals::instance(),
                &Core::HelpManager::Signals::documentationChanged,
                this,
                &FilterSettingsPage::updateFilterPage);

        updateFilterPage();
    }
    return m_widget;
}

void FilterSettingsPage::apply()
{
    if (m_widget->applySettings(LocalHelpManager::filterEngine()))
        emit filtersChanged();

    m_widget->readSettings(LocalHelpManager::filterEngine());
}

void FilterSettingsPage::finish()
{
    disconnect(Core::HelpManager::Signals::instance(),
               &Core::HelpManager::Signals::documentationChanged,
               this,
               &FilterSettingsPage::updateFilterPage);
    delete m_widget;
}

void FilterSettingsPage::updateFilterPage()
{
    m_widget->setAvailableComponents(LocalHelpManager::filterEngine()->availableComponents());
    m_widget->setAvailableVersions(LocalHelpManager::filterEngine()->availableVersions());
}

