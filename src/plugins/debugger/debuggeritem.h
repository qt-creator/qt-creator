// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kitaspect.h>

#include <utils/filepath.h>
#include <utils/environment.h>
#include <utils/store.h>

#include <QDateTime>

namespace Debugger {

namespace Internal {
class DebuggerConfigWidget;
class DebuggerItemConfigWidget;
class DebuggerItemModel;
} // namespace Internal

// -----------------------------------------------------------------------
// DebuggerItem
// -----------------------------------------------------------------------

class DEBUGGER_EXPORT DebuggerItem
{
public:
    struct TechnicalData
    {
        static Utils::Result<DebuggerItem::TechnicalData> extract(
            const Utils::FilePath &fromExecutable,
            const std::optional<Utils::Environment> &customEnvironment);
        bool isEmpty() const;

        DebuggerEngineType engineType = NoEngineType;
        ProjectExplorer::Abis abis;
        QString version;
    };

    static const ProjectExplorer::DetectionSource genericDetectionSource;

    DebuggerItem() = default;
    DebuggerItem(const Utils::Store &data);

    void createId();
    bool canClone() const { return true; }
    bool isValid() const;
    QString engineTypeName() const;

    Utils::Store toMap() const;

    QVariant id() const { return m_id; }

    QString displayName() const;
    QString unexpandedDisplayName() const { return m_unexpandedDisplayName; }
    void setUnexpandedDisplayName(const QString &unexpandedDisplayName);

    DebuggerEngineType engineType() const { return m_technicalData.engineType; }
    void setEngineType(const DebuggerEngineType &engineType);

    Utils::FilePath command() const { return m_command; }
    void setCommand(const Utils::FilePath &command);

    [[deprecated("Use getDetectionSource().isAutoDetected() instead")]] bool isAutoDetected() const;
    [[deprecated("Use setDetectionSource() instead")]] void setAutoDetected(bool isAutoDetected);

    QString version() const;
    void setVersion(const QString &version);

    const ProjectExplorer::Abis &abis() const { return m_technicalData.abis; }
    void setAbis(const ProjectExplorer::Abis &abis);
    void setAbi(const ProjectExplorer::Abi &abi);

    enum MatchLevel { DoesNotMatch, MatchesSomewhat, MatchesWell, MatchesPerfectly, MatchesPerfectlyInPath };
    MatchLevel matchTarget(const ProjectExplorer::Abi &targetAbi) const;

    QStringList abiNames() const;
    QDateTime lastModified() const;
    void setLastModified(const QDateTime &timestamp);

    // Keep enum sorted ascending by goodness.
    enum class Problem { NoEngine, InvalidCommand, InvalidWorkingDir, None };
    Problem problem() const;
    QIcon decoration() const;
    QString validityMessage() const;

    bool operator==(const DebuggerItem &other) const;
    bool operator!=(const DebuggerItem &other) const { return !operator==(other); }

    void reinitializeFromFile(QString *error = nullptr, Utils::Environment *env = nullptr);

    Utils::FilePath workingDirectory() const { return m_workingDirectory; }
    void setWorkingDirectory(const Utils::FilePath &workingPath) { m_workingDirectory = workingPath; }

    [[deprecated("Use setDetectionSource(DetectionSource) instead")]] void setDetectionSource(
        const QString &source);

    // Note: the earlier returned QString is the same as DetectionSource::id now
    ProjectExplorer::DetectionSource detectionSource() const;
    void setDetectionSource(const ProjectExplorer::DetectionSource &source);

    bool isGeneric() const;

    static bool addAndroidLldbPythonEnv(const Utils::FilePath &lldbCmd, Utils::Environment &env);
    static bool fixupAndroidLlldbPythonDylib(const Utils::FilePath &lldbCmd);

private:
    DebuggerItem(const QVariant &id);
    void initMacroExpander();

    QVariant m_id;
    QString m_unexpandedDisplayName;
    TechnicalData m_technicalData;
    Utils::FilePath m_command;
    Utils::FilePath m_workingDirectory;
    ProjectExplorer::DetectionSource m_detectionSource;
    QDateTime m_lastModified;

    friend class Internal::DebuggerConfigWidget;
    friend class Internal::DebuggerItemConfigWidget;
    friend class Internal::DebuggerItemModel;
};

} // namespace Debugger
