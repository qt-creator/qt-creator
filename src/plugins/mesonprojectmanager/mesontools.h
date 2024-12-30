// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/id.h>
#include <utils/store.h>

#include <QVersionNumber>

#include <optional>
#include <memory>

namespace Utils { class ProcessRunData; }

namespace MesonProjectManager::Internal {

enum class ToolType { Meson, Ninja };

class MesonToolWrapper final
{
public:
    MesonToolWrapper() = delete;
    explicit MesonToolWrapper(const Utils::Store &data);
    MesonToolWrapper(const QString &name,
                const Utils::FilePath &path,
                const Utils::Id &id = {},
                bool autoDetected = false);

    ~MesonToolWrapper();

    const QVersionNumber &version() const noexcept { return m_version; }
    bool isValid() const noexcept { return m_isValid; }
    bool autoDetected() const noexcept { return m_autoDetected; }
    Utils::Id id() const noexcept { return m_id; }
    Utils::FilePath exe() const noexcept { return m_exe; }
    QString name() const noexcept { return m_name; }

    void setName(const QString &newName) { m_name = newName; }
    void setExe(const Utils::FilePath &newExe);

    static QVersionNumber read_version(const Utils::FilePath &toolPath);

    Utils::Store toVariantMap() const;

    ToolType toolType() const { return ToolType::Meson; }

    Utils::ProcessRunData setup(const Utils::FilePath &sourceDirectory,
                                const Utils::FilePath &buildDirectory,
                                const QStringList &options = {}) const;
    Utils::ProcessRunData configure(const Utils::FilePath &sourceDirectory,
                                    const Utils::FilePath &buildDirectory,
                                    const QStringList &options = {}) const;
    Utils::ProcessRunData regenerate(const Utils::FilePath &sourceDirectory,
                                     const Utils::FilePath &buildDirectory) const;
    Utils::ProcessRunData introspect(const Utils::FilePath &sourceDirectory) const;

    Utils::ProcessRunData compile(const Utils::FilePath &buildDirectory,
                                  const QString& target, bool verbose = false) const;
    Utils::ProcessRunData compile(const Utils::FilePath &buildDirectory,
                                  bool verbose = false) const;

    Utils::ProcessRunData test(const Utils::FilePath &buildDirectory,
                               const QString& testSuite, bool verbose = false) const;

    Utils::ProcessRunData benchmark(const Utils::FilePath &buildDirectory,
                                    const QString& benchmark, bool verbose = false) const;

    Utils::ProcessRunData clean(const Utils::FilePath &buildDirectory,
                                bool verbose = false) const;

    Utils::ProcessRunData install(const Utils::FilePath &buildDirectory,
                                  bool verbose = false) const;

private:
    QVersionNumber m_version;
    bool m_isValid;
    bool m_autoDetected;
    Utils::Id m_id;
    Utils::FilePath m_exe;
    QString m_name;
};

bool run_meson(const Utils::ProcessRunData &runData, QIODevice *output = nullptr);

bool isSetup(const Utils::FilePath &buildPath);

std::optional<Utils::FilePath> findMeson();

class MesonTools : public QObject
{
    Q_OBJECT

    MesonTools() {}
    ~MesonTools() {}

public:
    using Tool_t = std::shared_ptr<MesonToolWrapper>;

    static void setTools(std::vector<Tool_t> &&tools);
    static const std::vector<Tool_t> &tools();

    static void updateTool(const Utils::Id &itemId,
                           const QString &name,
                           const Utils::FilePath &exe);
    static void removeTool(const Utils::Id &id);

    static std::shared_ptr<MesonToolWrapper> toolById(const Utils::Id &id);
    static std::shared_ptr<MesonToolWrapper> autoDetectedTool();

    static MesonTools *instance();

signals:
    void toolAdded(const Tool_t &tool);
    void toolRemoved(const Tool_t &tool);
};

} // MesonProjectManager::Internal
