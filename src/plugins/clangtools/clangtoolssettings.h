// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>

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

    void fromMap(const QVariantMap &map, const QString &prefix = QString());
    void toMap(QVariantMap &map, const QString &prefix = QString()) const;

    Utils::Id diagnosticConfigId() const;
    void setDiagnosticConfigId(const Utils::Id &id) { m_diagnosticConfigId = id; }

    bool buildBeforeAnalysis() const { return m_buildBeforeAnalysis; }
    void setBuildBeforeAnalysis(bool yesno) { m_buildBeforeAnalysis = yesno; }

    int parallelJobs() const { return m_parallelJobs; }
    void setParallelJobs(int jobs) { m_parallelJobs = jobs; }

    bool analyzeOpenFiles() const { return m_analyzeOpenFiles; }
    void setAnalyzeOpenFiles(bool analyzeOpenFiles) { m_analyzeOpenFiles = analyzeOpenFiles; }

    bool operator==(const RunSettings &other) const;

private:
    Utils::Id m_diagnosticConfigId;
    int m_parallelJobs = -1;
    bool m_buildBeforeAnalysis = true;
    bool m_analyzeOpenFiles = true;
};

class ClangToolsSettings : public QObject
{
    Q_OBJECT

public:
    static ClangToolsSettings *instance();
    void writeSettings();

    Utils::FilePath clangTidyExecutable() const { return m_clangTidyExecutable; }
    void setClangTidyExecutable(const Utils::FilePath &path);

    Utils::FilePath clazyStandaloneExecutable() const { return m_clazyStandaloneExecutable; }
    void setClazyStandaloneExecutable(const Utils::FilePath &path);

    CppEditor::ClangDiagnosticConfigs diagnosticConfigs() const { return m_diagnosticConfigs; }
    void setDiagnosticConfigs(const CppEditor::ClangDiagnosticConfigs &configs)
    { m_diagnosticConfigs = configs; }

    RunSettings runSettings() const { return m_runSettings; }
    void setRunSettings(const RunSettings &settings) { m_runSettings = settings; }

    static VersionAndSuffix clangTidyVersion();
    static QVersionNumber clazyVersion();

signals:
    void changed();

private:
    ClangToolsSettings();
    void readSettings();

    // Executables
    Utils::FilePath m_clangTidyExecutable;
    Utils::FilePath m_clazyStandaloneExecutable;

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
