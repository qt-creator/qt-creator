// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/clangdiagnosticconfigsselectionwidget.h>

#include <projectexplorer/project.h>

#include <utils/aspects.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
#include <QVersionNumber>

namespace ClangTools::Internal {

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
    void setRunSettingsEnabled(bool enabled);

    CppEditor::DiagnosticConfigIdAspect diagnosticConfigId{this};
    Utils::IntegerAspect parallelJobs{this};
    Utils::BoolAspect preferConfigFile{this};
    Utils::BoolAspect buildBeforeAnalysis{this};
    Utils::BoolAspect analyzeOpenFiles{this};
};

class ClangToolsSettings final : public RunSettings
{
    ClangToolsSettings();

public:
    static ClangToolsSettings *instance();
    void apply() override;
    void writeSettings() const override;

    // Executables
    Utils::FilePathAspect clangTidyExecutable{this};
    Utils::FilePathAspect clazyStandaloneExecutable{this};

    Utils::FilePath executable(CppEditor::ClangToolType tool) const;

    Utils::BoolAspect enableLowerClazyLevels{this};

    static VersionAndSuffix clangTidyVersion();
    static QVersionNumber clazyVersion();

private:
    // Version info. Ephemeral.
    VersionAndSuffix m_clangTidyVersion;
    QVersionNumber m_clazyVersion;
};

class Diagnostic;

class SuppressedDiagnostic
{
public:
    SuppressedDiagnostic(const Utils::FilePath &filePath, const QString &description, int uniquifier)
        : filePath(filePath), description(description), uniquifier(uniquifier) {}

    SuppressedDiagnostic(const Diagnostic &diag);

    friend bool operator==(const SuppressedDiagnostic &, const SuppressedDiagnostic &) = default;

    Utils::FilePath filePath; // Relative for files in project, absolute otherwise.
    QString description;
    int uniquifier;
};

using SuppressedDiagnosticsList = QList<SuppressedDiagnostic>;

class ClangToolsProjectSettings : public RunSettings
{
    Q_OBJECT

public:
    ClangToolsProjectSettings(ProjectExplorer::Project *project);
    ~ClangToolsProjectSettings() override;

    Utils::BoolAspect useGlobalSettings;

    QSet<Utils::FilePath> selectedDirs() const { return m_selectedDirs; }
    void setSelectedDirs(const QSet<Utils::FilePath> &value);

    QSet<Utils::FilePath> selectedFiles() const { return m_selectedFiles; }
    void setSelectedFiles(const QSet<Utils::FilePath> &value);

    SuppressedDiagnosticsList suppressedDiagnostics() const { return m_suppressedDiagnostics; }
    void addSuppressedDiagnostics(const SuppressedDiagnosticsList &diags);
    void addSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeAllSuppressedDiagnostics();

    using ClangToolsProjectSettingsPtr = std::shared_ptr<ClangToolsProjectSettings>;
    static ClangToolsProjectSettingsPtr getSettings(ProjectExplorer::Project *project);

signals:
    void suppressedDiagnosticsChanged();

private:
    void load();
    void store();

    ProjectExplorer::Project *m_project;
    QSet<Utils::FilePath> m_selectedDirs;
    QSet<Utils::FilePath> m_selectedFiles;
    SuppressedDiagnosticsList m_suppressedDiagnostics;
};

} // namespace ClangTools::Internal

Q_DECLARE_METATYPE(std::shared_ptr<ClangTools::Internal::ClangToolsProjectSettings>)
