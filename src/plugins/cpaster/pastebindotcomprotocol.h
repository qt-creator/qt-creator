// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "protocol.h"

namespace CodePaster {

class PasteBinDotComProtocol : public Protocol
{
public:
    PasteBinDotComProtocol();
    static QString protocolName();

    QtTaskTree::ExecutableItem fetchRecipe(const QString &id,
                                           const FetchHandler &handler) const override;
    QtTaskTree::ExecutableItem listRecipe(const ListHandler &handler) const override;
    QtTaskTree::ExecutableItem pasteRecipe(const PasteInputData &inputData,
                                           const PasteHandler &handler) const override;
};

} // CodePaster
