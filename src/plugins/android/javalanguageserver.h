// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageclient/languageclientsettings.h>

namespace Android {
namespace Internal {

class JLSSettings final : public LanguageClient::StdIOSettings
{
public:
    JLSSettings();

    bool applyFromSettingsWidget(QWidget *widget) final;
    QWidget *createSettingsWidget(QWidget *parent) const final;
    bool isValid() const final;
    Utils::Store toMap() const final;
    void fromMap(const Utils::Store &map) final;
    LanguageClient::BaseSettings *copy() const final;
    LanguageClient::Client *createClient(LanguageClient::BaseClientInterface *interface) const final;
    LanguageClient::BaseClientInterface *createInterface(
        ProjectExplorer::Project *project) const final;

    Utils::FilePath m_languageServer;

private:
    JLSSettings(const JLSSettings &other) = default;
};

} // namespace Internal
} // namespace Android
