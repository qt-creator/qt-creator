// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "protocol.h"

namespace CodePaster {

class UrlOpenProtocol : public NetworkProtocol
{
public:
    QString name() const override;
    unsigned capabilities() const override;
    void fetch(const QString &url) override;
    void paste(const QString &, ContentType, int, const QString &, const QString &, const QString &) override;

private:
    void fetchFinished();

    QNetworkReply *m_fetchReply = nullptr;
};

} // CodePaster
