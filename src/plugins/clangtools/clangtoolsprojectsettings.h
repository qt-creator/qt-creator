// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangtoolssettings.h"

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

namespace ClangTools {
namespace Internal {
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

class ClangToolsProjectSettings : public QObject
{
    Q_OBJECT

public:
    ClangToolsProjectSettings(ProjectExplorer::Project *project);
    ~ClangToolsProjectSettings() override;

    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setUseGlobalSettings(bool useGlobalSettings);

    RunSettings runSettings() const { return m_runSettings; }
    void setRunSettings(const RunSettings &settings);

    QSet<Utils::FilePath> selectedDirs() const { return m_selectedDirs; }
    void setSelectedDirs(const QSet<Utils::FilePath> &value);

    QSet<Utils::FilePath> selectedFiles() const { return m_selectedFiles; }
    void setSelectedFiles(const QSet<Utils::FilePath> &value);

    SuppressedDiagnosticsList suppressedDiagnostics() const { return m_suppressedDiagnostics; }
    void addSuppressedDiagnostics(const SuppressedDiagnosticsList &diags);
    void addSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeAllSuppressedDiagnostics();

    using ClangToolsProjectSettingsPtr = QSharedPointer<ClangToolsProjectSettings>;
    static ClangToolsProjectSettingsPtr getSettings(ProjectExplorer::Project *project);

signals:
    void suppressedDiagnosticsChanged();
    void changed();

private:
    void load();
    void store();

    ProjectExplorer::Project *m_project;

    bool m_useGlobalSettings = true;

    RunSettings m_runSettings;

    QSet<Utils::FilePath> m_selectedDirs;
    QSet<Utils::FilePath> m_selectedFiles;

    SuppressedDiagnosticsList m_suppressedDiagnostics;
};

} // namespace Internal
} // namespace ClangTools

Q_DECLARE_METATYPE(QSharedPointer<ClangTools::Internal::ClangToolsProjectSettings>)
