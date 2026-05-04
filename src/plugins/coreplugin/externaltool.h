// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>

namespace Core {

class CORE_EXPORT ExternalTool : public QObject
{
    Q_OBJECT

public:
    enum OutputHandling {
        Ignore,
        ShowInPane,
        ReplaceSelection
    };

    ExternalTool();
    explicit ExternalTool(const ExternalTool *other);
    ~ExternalTool() override;

    QString id() const;
    QString description() const;
    QString displayName() const;
    QString displayCategory() const;
    int order() const;
    OutputHandling outputHandling() const;
    OutputHandling errorHandling() const;
    bool modifiesCurrentDocument() const;

    Utils::FilePaths executables() const;
    QString arguments() const;
    QString input() const;
    Utils::FilePath workingDirectory() const;
    Utils::Id baseEnvironmentProviderId() const;
    Utils::Environment baseEnvironment() const;
    Utils::EnvironmentChanges environmentUserChanges() const;

    void setFilePath(const Utils::FilePath &filePath);
    void setPreset(std::shared_ptr<ExternalTool> preset);
    Utils::FilePath filePath() const;
    // all tools that are preset (changed or unchanged) have the original value here:
    std::shared_ptr<ExternalTool> preset() const;

    static Utils::Result<ExternalTool *> createFromXml(const QByteArray &xml,
                                                       const QString &locale = {});
    static Utils::Result<ExternalTool *> createFromFile(const Utils::FilePath &filePath,
                                                        const QString &locale = {});

    Utils::Result<> save() const;

    bool operator==(const ExternalTool &other) const;
    bool operator!=(const ExternalTool &other) const { return !((*this) == other); }
    ExternalTool &operator=(const ExternalTool &other);

    void setId(const QString &id);
    void setDisplayCategory(const QString &category);
    void setDisplayName(const QString &name);
    void setDescription(const QString &description);
    void setOutputHandling(OutputHandling handling);
    void setErrorHandling(OutputHandling handling);
    void setModifiesCurrentDocument(bool modifies);
    void setExecutables(const Utils::FilePaths &executables);
    void setArguments(const QString &arguments);
    void setInput(const QString &input);
    void setWorkingDirectory(const Utils::FilePath &workingDirectory);
    void setBaseEnvironmentProviderId(Utils::Id id);
    void setEnvironmentUserChanges(const Utils::EnvironmentChanges &items);

    void execute() const;

private:
    QString m_id;
    QString m_description;
    QString m_displayName;
    QString m_displayCategory;
    int m_order = -1;
    Utils::FilePaths m_executables;
    QString m_arguments;
    QString m_input;
    Utils::FilePath m_workingDirectory;
    Utils::Id m_baseEnvironmentProviderId;
    Utils::EnvironmentChanges m_environment;
    OutputHandling m_outputHandling = ShowInPane;
    OutputHandling m_errorHandling = ShowInPane;
    bool m_modifiesCurrentDocument = false;

    Utils::FilePath m_filePath;
    Utils::FilePath m_presetFileName;
    std::shared_ptr<ExternalTool> m_presetTool;
};

} // Core
