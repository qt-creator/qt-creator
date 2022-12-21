// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filtersettingspage.h"

#include "helpconstants.h"
#include "helptr.h"
#include "localhelpmanager.h"

#include <QHelpFilterEngine>
#include <QHelpFilterSettingsWidget>
#include <QVersionNumber>

using namespace Help::Internal;

FilterSettingsPage::FilterSettingsPage()
{
    setId("D.Filters");
    setDisplayName(Tr::tr("Filters"));
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

