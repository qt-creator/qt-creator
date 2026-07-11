// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

namespace VcsBase::Internal {

class ChangesViewFactory final : public Core::INavigationWidgetFactory
{
public:
    ChangesViewFactory();

private:
    Core::NavigationView createWidget() final;
};

} // namespace VcsBase::Internal
