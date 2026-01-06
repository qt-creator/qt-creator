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

    void fetch(const QString &id) override;
    QtTaskTree::ExecutableItem listRecipe(const ListHandler &handler) const override;
    void paste(const PasteInputData &inputData) override;

private:
    void fetchFinished();
    void pasteFinished();

    QNetworkReply *m_fetchReply = nullptr;
    QNetworkReply *m_pasteReply = nullptr;

    QString m_fetchId;
};

} // CodePaster
