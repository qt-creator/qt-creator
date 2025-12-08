// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <projectexplorer/kitaspect.h>

#include <texteditor/codeassist/keywordscompletionassist.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>

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

        friend bool operator==(const Version &v1, const Version &v2);
        friend bool operator!=(const Version &v1, const Version &v2);
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

        friend bool operator==(const CMakeTool::Generator &g1, const CMakeTool::Generator &g2);
        friend bool operator!=(const CMakeTool::Generator &g1, const CMakeTool::Generator &g2);
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

    void runCMake(Utils::Process &proc, const QStringList &args, int timeoutS = 5) const;
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
