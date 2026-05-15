// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gntools.h"

#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/settingsaccessor.h>
#include <utils/store.h>

#include <projectexplorer/kitaspect.h>

#include <QGuiApplication>

using namespace Core;
using namespace ProjectExplorer;
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

static GNTools::Tools s_tools;
static Id s_defaultGNId;

Id GNTools::defaultToolId()
{
    return s_defaultGNId;
}

void GNTools::setDefaultToolId(const Id &id)
{
    s_defaultGNId = id;
}

GNTools::Tool GNTools::autoDetectedTool()
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

void GNTools::setTools(Tools &&tools)
{
    std::swap(s_tools, tools);
    ensureAutoDetected();
}

int GNTools::toolCount()
{
    return static_cast<int>(s_tools.size());
}

QVariant GNTools::data(int row, int role)
{
    if (row < 0) {
        switch (role) {
        case Qt::DisplayRole:
            return Tr::tr("None");
        case KitAspect::IsNoneRole:
            return true;
        case KitAspect::IdRole:
            return {};
        }
        return {};
    }

    const Tool &tool = s_tools.at(row);
    switch (role) {
    case Qt::DisplayRole:
        return tool->name();
    case KitAspect::IdRole:
        return tool->id().toSetting();
    }
    return {};
}

const GNTools::Tools &GNTools::tools()
{
    return s_tools;
}

void GNTools::updateTool(const Id &itemId, const QString &name, const FilePath &exe)
{
    auto item = std::find_if(std::begin(s_tools), std::end(s_tools), [&itemId](const Tool &tool) {
        return tool->id() == itemId;
    });
    if (item != std::end(s_tools)) {
        (*item)->setExe(exe);
        (*item)->setName(name);
    } else {
        s_tools.emplace_back(std::make_shared<GNTool>(name, exe, itemId));
        emit instance()->toolsChanged();
    }
}

void GNTools::removeTool(const Id &id)
{
    auto item = Utils::take(s_tools, [&id](const auto &item) { return item->id() == id; });
    QTC_ASSERT(item, return);
    emit instance()->toolsChanged();
}

GNTools::Tool GNTools::toolById(const Id &id)
{
    const auto tool = std::find_if(std::cbegin(s_tools),
                                   std::cend(s_tools),
                                   [&id](const GNTools::Tool &tool) { return tool->id() == id; });
    if (tool != std::cend(s_tools))
        return *tool;
    return nullptr;
}

// Settings persistence

static Key entryName(int index)
{
    return numberedKey(Constants::ToolsSettings::ENTRY_KEY, index);
}

class GNToolsSettingsAccessor final : public UpgradingSettingsAccessor
{
public:
    GNToolsSettingsAccessor()
    {
        setDocType("QtCreatorGNTools");
        setApplicationDisplayName(QGuiApplication::applicationDisplayName());
        setBaseFilePath(ICore::userResourcePath(Constants::ToolsSettings::FILENAME));
        load();
        QObject::connect(ICore::instance(), &ICore::saveSettingsRequested, [this] { save(); });
    }

    void save()
    {
        Store data;
        int entryCount = 0;
        for (const GNTools::Tool &tool : GNTools::tools()) {
            data.insert(entryName(entryCount), variantFromStore(tool->toVariantMap()));
            ++entryCount;
        }
        data.insert(Constants::ToolsSettings::ENTRY_COUNT, entryCount);
        data.insert(Constants::ToolsSettings::DEFAULT_TOOL_KEY,
                    GNTools::defaultToolId().toSetting());
        saveSettings(data);
    }

    void load()
    {
        Store data = restoreSettings();
        int entryCount = data.value(Constants::ToolsSettings::ENTRY_COUNT, 0).toInt();
        GNTools::Tools result;
        for (int i = 0; i < entryCount; ++i) {
            Store store = storeFromVariant(data[entryName(i)]);
            if (!store.isEmpty())
                result.emplace_back(new GNTool(store));
        }
        GNTools::setTools(std::move(result));
        GNTools::setDefaultToolId(Id::fromSetting(
            data.value(Constants::ToolsSettings::DEFAULT_TOOL_KEY)));
    }
};

GNTools *GNTools::instance()
{
    static GNTools theGNTools;
    return &theGNTools;
}

void setupGNTools()
{
    static GNToolsSettingsAccessor accessor;
    GNTools::instance();
}

} // namespace GNProjectManager::Internal
