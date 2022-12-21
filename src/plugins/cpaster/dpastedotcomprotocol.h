// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "protocol.h"

namespace CodePaster {

class DPasteDotComProtocol : public NetworkProtocol
{
public:
    static QString protocolName();

private:
    QString name() const override { return protocolName(); }
    bool hasSettings() const override { return false; }
    unsigned capabilities() const override;
    void fetch(const QString &id) override;
    void fetchFinished(const QString &id, QNetworkReply * const reply, bool alreadyRedirected);
    void paste(const QString &text,
               ContentType ct = Text,
               int expiryDays = 1,
               const QString &username = QString(),
               const QString &comment = QString(),
               const QString &description = QString()) override;
    bool checkConfiguration(QString *errorMessage) override;

    static void reportError(const QString &message);
};

} // CodePaster
