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

class RunSettings
{
public:
    RunSettings();

    void fromMap(const Utils::Store &map, const Utils::Key &prefix = {});
    void toMap(Utils::Store &map, const Utils::Key &prefix = {}) const;

    Utils::Id diagnosticConfigId() const;
    void setDiagnosticConfigId(const Utils::Id &id) { m_diagnosticConfigId = id; }

    bool preferConfigFile() const { return m_preferConfigFile; }
    void setPreferConfigFile(bool yesno) { m_preferConfigFile = yesno; }

    bool buildBeforeAnalysis() const { return m_buildBeforeAnalysis; }
    void setBuildBeforeAnalysis(bool yesno) { m_buildBeforeAnalysis = yesno; }

    int parallelJobs() const { return m_parallelJobs; }
    void setParallelJobs(int jobs) { m_parallelJobs = jobs; }

    bool analyzeOpenFiles() const { return m_analyzeOpenFiles; }
    void setAnalyzeOpenFiles(bool analyzeOpenFiles) { m_analyzeOpenFiles = analyzeOpenFiles; }

    bool operator==(const RunSettings &other) const;

    bool hasConfigFileForSourceFile(const Utils::FilePath &sourceFile) const;

private:
    Utils::Id m_diagnosticConfigId;
    int m_parallelJobs = -1;
    bool m_preferConfigFile = true;
    bool m_buildBeforeAnalysis = true;
    bool m_analyzeOpenFiles = true;
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

    CppEditor::ClangDiagnosticConfigs diagnosticConfigs() const { return m_diagnosticConfigs; }
    void setDiagnosticConfigs(const CppEditor::ClangDiagnosticConfigs &configs)
    { m_diagnosticConfigs = configs; }

    RunSettings runSettings() const { return m_runSettings; }
    void setRunSettings(const RunSettings &settings) { m_runSettings = settings; }

    static VersionAndSuffix clangTidyVersion();
    static QVersionNumber clazyVersion();

private:
    void readSettings() override;

    // Diagnostic Configs
    CppEditor::ClangDiagnosticConfigs m_diagnosticConfigs;

    // Run settings
    RunSettings m_runSettings;

    // Version info. Ephemeral.
    VersionAndSuffix m_clangTidyVersion;
    QVersionNumber m_clazyVersion;
};

} // namespace Internal
} // namespace ClangTools
