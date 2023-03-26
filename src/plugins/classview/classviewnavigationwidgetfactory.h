// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

namespace ClassView {
namespace Internal {

class NavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    NavigationWidgetFactory();

    //! \implements Core::INavigationWidgetFactory::createWidget
    Core::NavigationView createWidget() override;

    //! \implements Core::INavigationWidgetFactory::saveSettings
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;

    //! \implements Core::INavigationWidgetFactory::restoreSettings
    void restoreSettings(QSettings *settings, int position, QWidget *widget) override;
};

} // namespace Internal
} // namespace ClassView
