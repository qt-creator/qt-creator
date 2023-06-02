// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <coreplugin/inavigationwidgetfactory.h>

#pragma once

namespace Core { class IDocument; }

namespace LanguageClient {

class Client;

class CallHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    CallHierarchyFactory();

    static bool supportsCallHierarchy(Client *client, const Core::IDocument *document);

    Core::NavigationView createWidget()  override;
};

} // namespace LanguageClient
