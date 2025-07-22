// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <projectexplorer/kitaspect.h>

#include <texteditor/codeassist/keywordscompletionassist.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>

#include <optional>

namespace Utils { class Process; }

namespace CMakeProjectManager {

namespace Internal {  class IntrospectionData;  }

struct CMAKE_EXPORT CMakeKeywords
{
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
    enum [[deprecated("Use DetectionSource::Type instead")]] Detection {
        ManualDetection,
        AutoDetection
    };

    enum ReaderType { FileApi };

    struct Version
    {
        int major = 0;
        int minor = 0;
        int patch = 0;
        QByteArray fullVersion;
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
    };

    using PathMapper = std::function<Utils::FilePath (const Utils::FilePath &)>;

    [[deprecated("Use CMakeTool(DetectionSource, Id) instead")]] explicit CMakeTool(
        Detection d, const Utils::Id &id);

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

    [[deprecated("Use detectionSource().isAutoDetected() instead")]] bool isAutoDetected() const;
    [[deprecated("Use setDetectionSource() instead")]] void setAutoDetected(bool autoDetected);
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void setPathMapper(const PathMapper &includePathMapper);
    PathMapper pathMapper() const;

    std::optional<ReaderType> readerType() const;

    static Utils::FilePath searchQchFile(const Utils::FilePath &executable);

    [[deprecated("Use setDetectionSource(DetectionSource) instead")]] void setDetectionSource(
        const QString &source);

    // Note: the earlier returned QString is the same as DetectionSource::id now
    ProjectExplorer::DetectionSource detectionSource() const;
    void setDetectionSource(const ProjectExplorer::DetectionSource &source);

    static QString documentationUrl(const Version &version, bool online);
    static void openCMakeHelpUrl(const CMakeTool *tool, const QString &linkUrl);

private:
    void readInformation() const;

    void runCMake(Utils::Process &proc, const QStringList &args, int timeoutS = 5) const;
    void parseFunctionDetailsOutput(const QString &output);
    QStringList parseVariableOutput(const QString &output);
    QStringList parseSyntaxHighlightingXml();

    void fetchFromCapabilities() const;
    void parseFromCapabilities(const QString &input) const;

    // Note: New items here need also be handled in CMakeToolItemModel::apply()
    // FIXME: Use a saner approach.
    Utils::Id m_id;
    QString m_displayName;
    Utils::FilePath m_executable;
    Utils::FilePath m_qchFilePath;

    ProjectExplorer::DetectionSource m_detectionSource;
    bool m_autoCreateBuildDirectory = false;

    std::optional<ReaderType> m_readerType;

    std::unique_ptr<Internal::IntrospectionData> m_introspection;

    PathMapper m_pathMapper;
};

} // namespace CMakeProjectManager
