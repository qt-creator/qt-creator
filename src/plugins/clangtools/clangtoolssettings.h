// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>

#include <utils/aspects.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>
#include <QPair>
#include <QString>
#include <QVersionNumber>

namespace ClangTools {
namespace Internal {

const char diagnosticConfigIdKey[] = "DiagnosticConfig";

using VersionAndSuffix = QPair<QVersionNumber, QString>;

class RunSettingsData
{
public:
    Utils::Id diagnosticConfigId;
    int parallelJobs = -1;
    bool preferConfigFile = true;
    bool buildBeforeAnalysis = true;
    bool analyzeOpenFiles = true;
    bool hasConfigFileForSourceFile(const Utils::FilePath &sourceFile) const;
};

class RunSettings : public Utils::AspectContainer
{
public:
    explicit RunSettings(const Utils::Key &prefix);

    RunSettingsData data() const;

    Utils::Id safeDiagnosticConfigId() const;
    bool hasConfigFileForSourceFile(const Utils::FilePath &sourceFile) const;

    Utils::TypedAspect<Utils::Id> diagnosticConfigId{this};
    Utils::IntegerAspect parallelJobs{this};
    Utils::BoolAspect preferConfigFile{this};
    Utils::BoolAspect buildBeforeAnalysis{this};
    Utils::BoolAspect analyzeOpenFiles{this};
};

class ClangToolsSettings : public Utils::AspectContainer
{
    ClangToolsSettings();

public:
    static ClangToolsSettings *instance();
    void writeSettings() const override;

    // Executables
    Utils::FilePathAspect clangTidyExecutable{this};
    Utils::FilePathAspect clazyStandaloneExecutable{this};

    Utils::FilePath executable(CppEditor::ClangToolType tool) const;
    void setExecutable(CppEditor::ClangToolType tool, const Utils::FilePath &path);

    Utils::BoolAspect enableLowerClazyLevels{this};

    CppEditor::ClangDiagnosticConfigs diagnosticConfigs() const { return m_diagnosticConfigs; }
    void setDiagnosticConfigs(const CppEditor::ClangDiagnosticConfigs &configs)
    { m_diagnosticConfigs = configs; }

    static VersionAndSuffix clangTidyVersion();
    static QVersionNumber clazyVersion();

    // Run settings
    RunSettings runSettings{{}}; // no prefix.

private:
    void readSettings() override;

    // Diagnostic Configs
    CppEditor::ClangDiagnosticConfigs m_diagnosticConfigs;

    // Version info. Ephemeral.
    VersionAndSuffix m_clangTidyVersion;
    QVersionNumber m_clazyVersion;
};

} // namespace Internal
} // namespace ClangTools
