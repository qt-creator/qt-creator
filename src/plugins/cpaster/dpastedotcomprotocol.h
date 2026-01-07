// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "protocol.h"

namespace CodePaster {

class DPasteDotComProtocol : public Protocol
{
public:
    DPasteDotComProtocol();
    static QString protocolName();

private:
    QtTaskTree::ExecutableItem fetchRecipe(const QString &id,
                                           const FetchHandler &handler) const override;
    void paste(const PasteInputData &inputData) override;
};

} // CodePaster
