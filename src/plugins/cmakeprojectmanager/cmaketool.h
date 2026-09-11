// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <projectexplorer/kitaspect.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>

#include <QFuture>

#include <optional>

namespace Utils { class Process; }

namespace CMakeProjectManager {

namespace Internal {  class IntrospectionData;  }

class CMAKE_EXPORT CMakeKeywords
{
public:
    QMap<QString, Utils::FilePath> variables;
    QMap<QString, Utils::FilePath> functions;
    QMap<QString, Utils::FilePath> properties;
    QSet<QString> generatorExpressions;
    QMap<QString, Utils::FilePath> environmentVariables;
    QMap<QString, Utils::FilePath> directoryProperties;
    QMap<QString, Utils::FilePath> sourceProperties;
    QMap<QString, Utils::FilePath> targetProperties;
    QMap<QString, Utils::FilePath> testProperties;
    QMap<QString, Utils::FilePath> includeStandardModules;
    QMap<QString, Utils::FilePath> findModules;
    QMap<QString, Utils::FilePath> policies;
    QMap<QString, QStringList> functionArgs;
};

class CMAKE_EXPORT CMakeTool
{
public:
    struct Version
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        QByteArray fullVersion;

    private:
        friend bool operator==(const Version &, const Version &) = default;
    };

    class Generator
    {
    public:
        Generator(const QString &n, const QStringList &eg, bool pl = true, bool ts = true) :
            name(n), extraGenerators(eg), supportsPlatform(pl), supportsToolset(ts)
        { }

        QString name;
        QStringList extraGenerators;
        bool supportsPlatform = true;
        bool supportsToolset = true;

        bool matches(const QString &n) const;

    private:
        friend bool operator==(const CMakeTool::Generator &, const CMakeTool::Generator &) = default;
    };

    explicit CMakeTool(const Utils::Store &map, bool fromSdk);
    explicit CMakeTool(const ProjectExplorer::DetectionSource &d, const Utils::Id &id);

    ~CMakeTool();

    static Utils::Id createId();

    bool isValid() const;

    Utils::Id id() const { return m_id; }
    Utils::Store toMap () const;

    void setFilePath(const Utils::FilePath &executable);
    Utils::FilePath filePath() const;
    Utils::FilePath cmakeExecutable() const;
    void setQchFilePath(const Utils::FilePath &path);
    Utils::FilePath qchFilePath() const;
    static Utils::FilePath cmakeExecutable(const Utils::FilePath &path);
    bool isAutoRun() const;
    bool autoCreateBuildDirectory() const;
    QList<Generator> supportedGenerators() const;
    CMakeKeywords keywords();

    // The keywords as far as they have been read: nothing while the reading
    // is still going on, so that asking for them never waits for it.
    std::optional<CMakeKeywords> keywordsIfRead();

    // Reading them takes a process of CMake and the files of its Help, and
    // whoever asks for them first waits for that.  Reading them before
    // they are asked for keeps that wait out of an editor.  The future it
    // hands out is done once they are there.
    QFuture<void> readKeywords();

    bool hasFileApi() const;
    Version version() const;
    QString versionDisplay() const;

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    static Utils::FilePath searchQchFile(const Utils::FilePath &executable);

    // Note: the earlier returned QString is the same as DetectionSource::id now
    ProjectExplorer::DetectionSource detectionSource() const;
    void setDetectionSource(const ProjectExplorer::DetectionSource &source);

private:
    void readInformation() const;

    // Which module of CMake documents which of the commands the modules
    // provide.  Nothing waits for this: a command whose module has not
    // been read yet is one that no module documents, until it has.
    void readModuleCommands();

    QStringList parseSyntaxHighlightingXml();

    void fetchFromCapabilities() const;

    // Note: New items here need also be handled in CMakeToolItemModel::apply()
    // FIXME: Use a saner approach.
    Utils::Id m_id;
    QString m_displayName;
    Utils::FilePath m_executable;
    Utils::FilePath m_qchFilePath;

    ProjectExplorer::DetectionSource m_detectionSource;
    bool m_autoCreateBuildDirectory = false;

    std::unique_ptr<Internal::IntrospectionData> m_introspection;
};

} // namespace CMakeProjectManager
