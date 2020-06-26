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

    Utils::FilePath filePath; // Relative for files in project, absolute otherwise.
    QString description;
    int uniquifier;
};

inline bool operator==(const SuppressedDiagnostic &d1, const SuppressedDiagnostic &d2)
{
    return d1.filePath == d2.filePath
        && d1.description == d2.description
        && d1.uniquifier == d2.uniquifier;
}

using SuppressedDiagnosticsList = QList<SuppressedDiagnostic>;

class ClangToolsProjectSettings : public QObject
{
    Q_OBJECT

public:
    ClangToolsProjectSettings(ProjectExplorer::Project *project);
    ~ClangToolsProjectSettings() override;

    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setUseGlobalSettings(bool useGlobalSettings) { m_useGlobalSettings = useGlobalSettings; }

    RunSettings runSettings() const { return m_runSettings; }
    void setRunSettings(const RunSettings &settings) { m_runSettings = settings; }

    QSet<Utils::FilePath> selectedDirs() const { return m_selectedDirs; }
    void setSelectedDirs(const QSet<Utils::FilePath> &value) { m_selectedDirs = value; }

    QSet<Utils::FilePath> selectedFiles() const { return m_selectedFiles; }
    void setSelectedFiles(const QSet<Utils::FilePath> &value) { m_selectedFiles = value; }

    SuppressedDiagnosticsList suppressedDiagnostics() const { return m_suppressedDiagnostics; }
    void addSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    void removeAllSuppressedDiagnostics();

    using ClangToolsProjectSettingsPtr = QSharedPointer<ClangToolsProjectSettings>;
    static ClangToolsProjectSettingsPtr getSettings(ProjectExplorer::Project *project);

signals:
    void suppressedDiagnosticsChanged();

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
