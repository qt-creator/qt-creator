// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "protocol.h"
#include "fileshareprotocolsettingspage.h"

namespace CodePaster {

class FileShareProtocolSettingsPage;

/* FileShareProtocol: Allows for pasting via a shared network
 * drive by writing XML files. */

class FileShareProtocol : public Protocol
{
public:
    FileShareProtocol();
    ~FileShareProtocol() override;

    QString name() const override;
    unsigned capabilities() const override;
    bool hasSettings() const override;
    Core::IOptionsPage *settingsPage() const override;

    bool checkConfiguration(QString *errorMessage = nullptr) override;
    void fetch(const QString &id) override;
    void list() override;
    void paste(const QString &text,
               ContentType ct = Text, int expiryDays = 1,
               const QString &username = QString(),
               const QString &comment = QString(),
               const QString &description = QString()) override;

private:
    FileShareProtocolSettings m_settings;
    FileShareProtocolSettingsPage *m_settingsPage;
};

} // CodePaster
