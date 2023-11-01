// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>

namespace CppEditor::Internal {

class CppTypeHierarchyFactory : public Core::INavigationWidgetFactory
{
public:
    CppTypeHierarchyFactory();

    Core::NavigationView createWidget()  override;
};

} // CppEditor::Internal
