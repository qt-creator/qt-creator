// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "protocol.h"

namespace CodePaster {

class UrlOpenProtocol : public Protocol
{
public:
    UrlOpenProtocol();

    QtTaskTree::ExecutableItem fetchRecipe(const QString &id,
                                           const FetchHandler &handler) const override;
    void paste(const PasteInputData &) override {}
};

} // CodePaster
