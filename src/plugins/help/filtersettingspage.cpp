// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filtersettingspage.h"

#include "helpconstants.h"
#include "helptr.h"
#include "localhelpmanager.h"

#include <QHelpFilterEngine>
#include <QHelpFilterSettingsWidget>
#include <QVBoxLayout>
#include <QVersionNumber>

namespace Help::Internal {

class FilterSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    FilterSettingsPageWidget(FilterSettingsPage *page) : m_page(page)
    {
        LocalHelpManager::setupGuiHelpEngine();
        m_widget = new QHelpFilterSettingsWidget;
        m_widget->readSettings(LocalHelpManager::filterEngine());

        auto vbox = new QVBoxLayout(this);
        vbox->addWidget(m_widget);
        vbox->setContentsMargins(0, 0, 0, 0);

        connect(Core::HelpManager::Signals::instance(),
                &Core::HelpManager::Signals::documentationChanged,
                this,
                &FilterSettingsPageWidget::updateFilterPage);

        updateFilterPage();
    }

    void apply() final
    {
        if (m_widget->applySettings(LocalHelpManager::filterEngine()))
            emit m_page->filtersChanged();

        m_widget->readSettings(LocalHelpManager::filterEngine());
    }

    void finish() final
    {
        disconnect(Core::HelpManager::Signals::instance(),
                   &Core::HelpManager::Signals::documentationChanged,
                   this,
                   &FilterSettingsPageWidget::updateFilterPage);
    }

    void updateFilterPage()
    {
        m_widget->setAvailableComponents(LocalHelpManager::filterEngine()->availableComponents());
        m_widget->setAvailableVersions(LocalHelpManager::filterEngine()->availableVersions());
    }

    FilterSettingsPage *m_page;
    QHelpFilterSettingsWidget *m_widget;
};

FilterSettingsPage::FilterSettingsPage()
{
    setId("D.Filters");
    setDisplayName(Tr::tr("Filters"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setWidgetCreator([this] { return new FilterSettingsPageWidget(this); });
}

} // Help::Internal
