// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageclient/languageclientsettings.h>

#include <utils/aspects.h>

#include <QVersionNumber>
#include <QWidget>

namespace QmlJSEditor {

class QmllsClientSettings : public LanguageClient::BaseSettings
{
public:
    enum ExecutableSelection { FromQtKit, FromLatestQtKit, FromUser };
    static const inline QVersionNumber mininumQmllsVersion = QVersionNumber(6, 8);

    QmllsClientSettings();
    BaseSettings *create() const override { return new QmllsClientSettings; }

    QWidget *createSettingsWidget(QWidget *parent = nullptr) override;

    void fromMap(const Utils::Store &map) override;

    bool isValidOnBuildConfiguration(ProjectExplorer::BuildConfiguration *bc) const override;

    // helpers:
    bool isEnabledOnProjectFile(const Utils::FilePath &file) const;
    bool useQmllsWithBuiltinCodemodelOnProject(ProjectExplorer::Project *project,
                                               const Utils::FilePath &file) const;

    Utils::TypedSelectionAspect<ExecutableSelection> executableSelection{this};
    Utils::BoolAspect ignoreMinimumQmllsVersion{this};
    Utils::BoolAspect useQmllsSemanticHighlighting{this};
    Utils::BoolAspect disableBuiltinCodemodel{this};
    Utils::BoolAspect generateQmllsIniFiles{this};
    Utils::BoolAspect enableCMakeBuilds{this};
    Utils::FilePathAspect executable{this};

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
