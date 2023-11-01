// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

namespace ClassView::Internal {

class NavigationWidgetFactory : public Core::INavigationWidgetFactory
{
public:
    NavigationWidgetFactory();

    Core::NavigationView createWidget() override;
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
};

} // ClassView::Internal
