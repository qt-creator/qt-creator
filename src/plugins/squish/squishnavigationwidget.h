// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

namespace Squish::Internal {

class SquishNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
public:
    SquishNavigationWidgetFactory();

private:
    Core::NavigationView createWidget() override;
};

} // Squish::Internal
