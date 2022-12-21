// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/iwelcomepage.h>

#include <QCoreApplication>

namespace Marketplace {
namespace Internal {

class QtMarketplaceWelcomePage : public Core::IWelcomePage
{
    Q_DECLARE_TR_FUNCTIONS(Marketplace::Internal::QtMarketplaceWelcomePage)
public:
    QtMarketplaceWelcomePage() = default;

    QString title() const final;
    int priority() const final;
    Utils::Id id() const final;
    QWidget *createWidget() const final;
};

} // namespace Internal
} // namespace Marketplace
