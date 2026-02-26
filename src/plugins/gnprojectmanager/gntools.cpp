// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gntools.h"

#include "gnpluginconstants.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace GNProjectManager::Internal {

GNTool::GNTool(const Store &data)
    : GNTool(data[Constants::ToolsSettings::NAME_KEY].toString(),
             FilePath::fromSettings(data[Constants::ToolsSettings::EXE_KEY]),
             Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]),
             data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool())
{}

GNTool::GNTool(const QString &name, const FilePath &path, const Id &id, bool autoDetected)
    : m_version(readVersion(path))
    , m_isValid{path.exists() && !m_version.isNull()}
    , m_autoDetected{autoDetected}
    , m_id{id.isValid() ? id : Id::generate()}
    , m_exe{path}
    , m_name{name}
{
    QTC_ASSERT(m_id.isValid(), m_id = Id::generate());
}

GNTool::~GNTool() = default;

void GNTool::setExe(const FilePath &newExe)
{
    m_exe = newExe;
    m_version = readVersion(m_exe);
    m_isValid = m_exe.exists() && !m_version.isNull();
}

QVersionNumber GNTool::readVersion(const FilePath &toolPath)
{
    if (toolPath.isExecutableFile()) {
        Process process;
        process.setCommand({toolPath, {"--version"}});
        process.start();
        if (process.waitForFinished()) {
            // GN --version outputs something like "2100 (5765e437)"
            // We parse the first number as the version
            const QString output = process.cleanedStdOut().trimmed();
            // Try to extract a version number
            const QStringList parts = output.split(' ');
            if (!parts.isEmpty()) {
                bool ok = false;
                int ver = parts.first().toInt(&ok);
                if (ok)
                    return QVersionNumber(ver);
                // Fall back to standard version parsing
                return QVersionNumber::fromString(parts.first());
            }
        }
    }
    return {};
}

Store GNTool::toVariantMap() const
{
    Store data;
    data.insert(Constants::ToolsSettings::NAME_KEY, m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, m_exe.toSettings());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, m_id.toSetting());
    return data;
}

std::optional<FilePath> findGN()
{
    const Environment systemEnvironment = Environment::systemEnvironment();
    FilePath gnPath = systemEnvironment.searchInPath("gn");
    if (gnPath.exists())
        return gnPath;
    return std::nullopt;
}

// GNTools

std::vector<GNTools::Tool_t> s_tools;

GNTools::Tool_t GNTools::autoDetectedTool()
{
    for (const auto &tool : s_tools) {
        if (tool->autoDetected())
            return tool;
    }
    return nullptr;
}

static void ensureAutoDetected()
{
    if (GNTools::autoDetectedTool())
        return;

    if (const std::optional<FilePath> path = findGN()) {
        s_tools.emplace_back(
            std::make_shared<GNTool>(QString("System GN at %1").arg(path->toUserOutput()),
                                     *path,
                                     Id{},
                                     true));
    }
}

void GNTools::setTools(std::vector<GNTools::Tool_t> &&tools)
{
    std::swap(s_tools, tools);
    ensureAutoDetected();
}

const std::vector<GNTools::Tool_t> &GNTools::tools()
{
    return s_tools;
}

void GNTools::updateTool(const Id &itemId, const QString &name, const FilePath &exe)
{
    auto item = std::find_if(std::begin(s_tools), std::end(s_tools), [&itemId](const Tool_t &tool) {
        return tool->id() == itemId;
    });
    if (item != std::end(s_tools)) {
        (*item)->setExe(exe);
        (*item)->setName(name);
    } else {
        s_tools.emplace_back(std::make_shared<GNTool>(name, exe, itemId));
        emit instance()->toolAdded(s_tools.back());
    }
}

void GNTools::removeTool(const Id &id)
{
    auto item = Utils::take(s_tools, [&id](const auto &item) { return item->id() == id; });
    QTC_ASSERT(item, return);
    emit instance()->toolRemoved(*item);
}

std::shared_ptr<GNTool> GNTools::toolById(const Id &id)
{
    const auto tool = std::find_if(std::cbegin(s_tools),
                                   std::cend(s_tools),
                                   [&id](const GNTools::Tool_t &tool) { return tool->id() == id; });
    if (tool != std::cend(s_tools))
        return *tool;
    return nullptr;
}

GNTools *GNTools::instance()
{
    static GNTools inst;
    return &inst;
}

} // namespace GNProjectManager::Internal
