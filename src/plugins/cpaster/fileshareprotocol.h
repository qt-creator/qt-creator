// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "protocol.h"

namespace CodePaster {

/* FileShareProtocol: Allows for pasting via a shared network
 * drive by writing XML files. */

class FileShareProtocol : public Protocol
{
public:
    FileShareProtocol();

    QtTaskTree::ExecutableItem fetchRecipe(const QString &id,
                                           const FetchHandler &handler) const override;
    QtTaskTree::ExecutableItem listRecipe(const ListHandler &handler) const override;
    QtTaskTree::ExecutableItem pasteRecipe(const PasteInputData &inputData,
                                           const PasteHandler &handler) const override;
};

} // CodePaster
