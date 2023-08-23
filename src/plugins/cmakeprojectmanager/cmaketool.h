// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <texteditor/codeassist/keywordscompletionassist.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/storage.h>

#include <optional>

namespace Utils { class Process; }

namespace CMakeProjectManager {

namespace Internal {  class IntrospectionData;  }

class CMAKE_EXPORT CMakeTool
{
public:
    enum Detection { ManualDetection, AutoDetection };

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

        bool matches(const QString &n, const QString &ex = QString()) const;
    };

    using PathMapper = std::function<Utils::FilePath (const Utils::FilePath &)>;

    explicit CMakeTool(Detection d, const Utils::Id &id);
    explicit CMakeTool(const QVariantMap &map, bool fromSdk);
    ~CMakeTool();

    static Utils::Id createId();

    bool isValid() const;

    Utils::Id id() const { return m_id; }
    Utils::Storage toMap () const;

    void setAutorun(bool autoRun) { m_isAutoRun = autoRun; }

    void setFilePath(const Utils::FilePath &executable);
    Utils::FilePath filePath() const;
    Utils::FilePath cmakeExecutable() const;
    void setQchFilePath(const Utils::FilePath &path);
    Utils::FilePath qchFilePath() const;
    static Utils::FilePath cmakeExecutable(const Utils::FilePath &path);
    bool isAutoRun() const;
    bool autoCreateBuildDirectory() const;
    QList<Generator> supportedGenerators() const;
    TextEditor::Keywords keywords();
    bool hasFileApi() const;
    Version version() const;
    QString versionDisplay() const;

    bool isAutoDetected() const;
    QString displayName() const;
    void setDisplayName(const QString &displayName);

    void setPathMapper(const PathMapper &includePathMapper);
    PathMapper pathMapper() const;

    std::optional<ReaderType> readerType() const;

    static Utils::FilePath searchQchFile(const Utils::FilePath &executable);

    QString detectionSource() const { return m_detectionSource; }
    void setDetectionSource(const QString &source) { m_detectionSource = source; }

    static QString documentationUrl(const Version &version, bool online);
    static void openCMakeHelpUrl(const CMakeTool *tool, const QString &linkUrl);

private:
    void readInformation() const;

    void runCMake(Utils::Process &proc, const QStringList &args, int timeoutS = 1) const;
    void parseFunctionDetailsOutput(const QString &output);
    QStringList parseVariableOutput(const QString &output);

    void fetchFromCapabilities() const;
    void parseFromCapabilities(const QString &input) const;

    // Note: New items here need also be handled in CMakeToolItemModel::apply()
    // FIXME: Use a saner approach.
    Utils::Id m_id;
    QString m_displayName;
    Utils::FilePath m_executable;
    Utils::FilePath m_qchFilePath;

    bool m_isAutoRun = true;
    bool m_isAutoDetected = false;
    QString m_detectionSource;
    bool m_autoCreateBuildDirectory = false;

    std::optional<ReaderType> m_readerType;

    std::unique_ptr<Internal::IntrospectionData> m_introspection;

    PathMapper m_pathMapper;
};

} // namespace CMakeProjectManager
