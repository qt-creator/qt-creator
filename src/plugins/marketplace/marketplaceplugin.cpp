// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtmarketplacewelcomepage.h"

#include <extensionsystem/iplugin.h>

namespace Marketplace::Internal {

class MarketplacePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Marketplace.json")

    void initialize() final
    {
        setupQtMarketPlaceWelcomePage(this);
    }
};

} // Marketplace::Internal

#include "marketplaceplugin.moc"
