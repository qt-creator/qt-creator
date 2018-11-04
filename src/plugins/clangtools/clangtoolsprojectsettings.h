/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QObject>

#include <coreplugin/id.h>
#include <projectexplorer/project.h>

#include <utils/fileutils.h>

namespace ClangTools {
namespace Internal {
class Diagnostic;

class SuppressedDiagnostic
{
public:
    SuppressedDiagnostic(const Utils::FileName &filePath, const QString &description,
                         const QString &contextKind, const QString &context, int uniquifier)
        : filePath(filePath)
        , description(description)
        , contextKind(contextKind)
        , context(context)
        , uniquifier(uniquifier)
    {
    }

    SuppressedDiagnostic(const Diagnostic &diag);

    Utils::FileName filePath; // Relative for files in project, absolute otherwise.
    QString description;
    QString contextKind;
    QString context;
    int uniquifier;
};

inline bool operator==(const SuppressedDiagnostic &d1, const SuppressedDiagnostic &d2)
{
    return d1.filePath == d2.filePath && d1.description == d2.description
            && d1.contextKind == d2.contextKind && d1.context == d2.context
            && d1.uniquifier == d2.uniquifier;
}

using SuppressedDiagnosticsList = QList<SuppressedDiagnostic>;

class ClangToolsProjectSettings : public QObject
{
    Q_OBJECT

public:
    ClangToolsProjectSettings(ProjectExplorer::Project *project);
    ~ClangToolsProjectSettings() override;

    bool useGlobalSettings() const;
    void setUseGlobalSettings(bool useGlobalSettings);

    Core::Id diagnosticConfig() const;
    void setDiagnosticConfig(const Core::Id &diagnosticConfig);

    bool buildBeforeAnalysis() const;
    void setBuildBeforeAnalysis(bool build);

    QSet<Utils::FileName> selectedDirs() const { return m_selectedDirs; }
    void setSelectedDirs(const QSet<Utils::FileName> &value) { m_selectedDirs = value; }

    QSet<Utils::FileName> selectedFiles() const { return m_selectedFiles; }
    void setSelectedFiles(const QSet<Utils::FileName> &value) { m_selectedFiles = value; }

    SuppressedDiagnosticsList suppressedDiagnostics() const { return m_suppressedDiagnostics; }
    void addSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeAllSuppressedDiagnostics();

signals:
    void suppressedDiagnosticsChanged();

private:
    void load();
    void store();

    ProjectExplorer::Project *m_project;
    bool m_useGlobalSettings = true;
    Core::Id m_diagnosticConfig;
    QSet<Utils::FileName> m_selectedDirs;
    QSet<Utils::FileName> m_selectedFiles;
    SuppressedDiagnosticsList m_suppressedDiagnostics;
    bool m_buildBeforeAnalysis = true;
};

class ClangToolsProjectSettingsManager
{
public:
    ClangToolsProjectSettingsManager();

    static ClangToolsProjectSettings *getSettings(ProjectExplorer::Project *project);

private:
    static void handleProjectToBeRemoved(ProjectExplorer::Project *project);

    using SettingsMap = QHash<ProjectExplorer::Project *, QSharedPointer<ClangToolsProjectSettings>>;
    static SettingsMap m_settings;
};

} // namespace Internal
} // namespace ClangTools
