// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <coreplugin/inavigationwidgetfactory.h>

#pragma once

namespace LanguageClient {

class CallHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    CallHierarchyFactory();

    Core::NavigationView createWidget()  override;
};

} // namespace LanguageClient
