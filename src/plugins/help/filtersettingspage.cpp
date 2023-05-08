// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filtersettingspage.h"

#include "helpconstants.h"
#include "helptr.h"
#include "localhelpmanager.h"

#include <utils/layoutbuilder.h>

#include <QHelpFilterEngine>
#include <QHelpFilterSettingsWidget>
#include <QVersionNumber>

namespace Help::Internal {

class FilterSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    FilterSettingsPageWidget(const std::function<void()> &onChanged)
    {
        LocalHelpManager::setupGuiHelpEngine();

        auto widget = new QHelpFilterSettingsWidget;
        widget->readSettings(LocalHelpManager::filterEngine());

        Layouting::Column {
            Layouting::noMargin,
            widget
        }.attachTo(this);

        auto updateFilterPage = [widget] {
            widget->setAvailableComponents(LocalHelpManager::filterEngine()->availableComponents());
            widget->setAvailableVersions(LocalHelpManager::filterEngine()->availableVersions());
        };

        auto connection = connect(Core::HelpManager::Signals::instance(),
                                  &Core::HelpManager::Signals::documentationChanged,
                                  this,
                                  updateFilterPage);
        updateFilterPage();

        setOnApply([widget, onChanged] {
            if (widget->applySettings(LocalHelpManager::filterEngine()))
                onChanged();
            widget->readSettings(LocalHelpManager::filterEngine());
        });

        setOnFinish([connection] {  disconnect(connection); });
    }
};

FilterSettingsPage::FilterSettingsPage(const std::function<void ()> &onChanged)
{
    setId("D.Filters");
    setDisplayName(Tr::tr("Filters"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setWidgetCreator([onChanged] { return new FilterSettingsPageWidget(onChanged); });
}

} // Help::Internal
