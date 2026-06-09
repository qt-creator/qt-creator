// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangtoolssettings.h"

#include <projectexplorer/project.h>

#include <utils/aspects.h>

namespace ClangTools::Internal {

class Diagnostic;

class SuppressedDiagnostic
{
public:
    SuppressedDiagnostic(const Utils::FilePath &filePath, const QString &description, int uniquifier)
        : filePath(filePath)
        , description(description)
        , uniquifier(uniquifier)
    {
    }

    SuppressedDiagnostic(const Diagnostic &diag);

    friend bool operator==(const SuppressedDiagnostic &d1, const SuppressedDiagnostic &d2)
    {
        return d1.filePath == d2.filePath
                && d1.description == d2.description
                && d1.uniquifier == d2.uniquifier;
    }

    Utils::FilePath filePath; // Relative for files in project, absolute otherwise.
    QString description;
    int uniquifier;
};

using SuppressedDiagnosticsList = QList<SuppressedDiagnostic>;

class ClangToolsProjectSettings : public Utils::AspectContainer
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

    RunSettings runSettings;

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

} // ClangTools::Internal

Q_DECLARE_METATYPE(std::shared_ptr<ClangTools::Internal::ClangToolsProjectSettings>)
