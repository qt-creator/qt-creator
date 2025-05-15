// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageclient/languageclientsettings.h>
#include <QVersionNumber>

#include <QWidget>

namespace QmlJSEditor {

class QmllsClientSettings : public LanguageClient::BaseSettings
{
public:
    static const inline QVersionNumber mininumQmllsVersion = QVersionNumber(6, 8);

    QmllsClientSettings();
    BaseSettings *copy() const override { return new QmllsClientSettings(*this); }

    QWidget *createSettingsWidget(QWidget *parent = nullptr) const override;
    bool applyFromSettingsWidget(QWidget *widget) override;

    void toMap(Utils::Store &map) const override;
    void fromMap(const Utils::Store &map) override;

    bool isValidOnBuildConfiguration(ProjectExplorer::BuildConfiguration *bc) const override;

    // helpers:
    bool isEnabledOnProjectFile(const Utils::FilePath &file) const;
    bool useQmllsWithBuiltinCodemodelOnProject(ProjectExplorer::Project *project,
                                               const Utils::FilePath &file) const;

    bool m_useLatestQmlls = false;
    bool m_ignoreMinimumQmllsVersion = false;
    bool m_useQmllsSemanticHighlighting = false;
    bool m_disableBuiltinCodemodel = false;
    bool m_generateQmllsIniFiles = false;

protected:
    LanguageClient::BaseClientInterface *createInterface(
        ProjectExplorer::BuildConfiguration *) const override;
    LanguageClient::Client *createClient(
        LanguageClient::BaseClientInterface *interface) const override;
};

QmllsClientSettings *qmllsSettings();
void registerQmllsSettings();
void setupQmllsClient();

} // namespace QmlJSEditor
