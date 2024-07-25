// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesoninfoparser.h"

#include "buildoptions.h"
#include "common.h"
#include "mesonpluginconstants.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <memory>
#include <optional>

using namespace Utils;

namespace MesonProjectManager::Internal::MesonInfoParser {

class BuildOptionsParser
{
    static inline std::unique_ptr<BuildOption> load_option(const QJsonObject &option)
    {
        const auto type = option["type"].toString();
        if (type == "string")
            return std::make_unique<StringBuildOption>(option["name"].toString(),
                                                       option["section"].toString(),
                                                       option["description"].toString(),
                                                       option["value"]);
        if (type == "boolean")
            return std::make_unique<BooleanBuildOption>(option["name"].toString(),
                                                        option["section"].toString(),
                                                        option["description"].toString(),
                                                        option["value"]);
        if (type == "combo")
            return std::make_unique<ComboBuildOption>(option["name"].toString(),
                                                      option["section"].toString(),
                                                      option["description"].toString(),
                                                      option["choices"].toVariant().toStringList(),
                                                      option["value"]);
        if (type == "integer")
            return std::make_unique<IntegerBuildOption>(option["name"].toString(),
                                                        option["section"].toString(),
                                                        option["description"].toString(),
                                                        option["value"]);
        if (type == "array")
            return std::make_unique<ArrayBuildOption>(option["name"].toString(),
                                                      option["section"].toString(),
                                                      option["description"].toString(),
                                                      option["value"].toVariant());
        if (type == "feature")
            return std::make_unique<FeatureBuildOption>(option["name"].toString(),
                                                        option["section"].toString(),
                                                        option["description"].toString(),
                                                        option["value"]);
        return std::make_unique<UnknownBuildOption>(option["name"].toString(),
                                                    option["section"].toString(),
                                                    option["description"].toString());
    }

    static inline std::vector<std::unique_ptr<BuildOption>> load_options(const QJsonArray &arr)
    {
        std::vector<std::unique_ptr<BuildOption>> buildOptions;
        std::transform(std::cbegin(arr),
                       std::cend(arr),
                       std::back_inserter(buildOptions),
                       [](const auto &option) { return load_option(option.toObject()); });
        return buildOptions;
    }

    std::vector<std::unique_ptr<BuildOption>> m_buildOptions;

public:
    BuildOptionsParser(const FilePath &buildDir)
    {
        FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_BUIDOPTIONS;
        auto arr = load<QJsonArray>(path.toFSPathString());
        if (arr)
            m_buildOptions = load_options(*arr);
    }

    BuildOptionsParser(const QJsonDocument &js)
    {
        auto obj = get<QJsonArray>(js.object(), "buildoptions");
        if (obj)
            m_buildOptions = load_options(*obj);
    }

    inline std::vector<std::unique_ptr<BuildOption>> takeBuildOptions()
    {
        return std::move(m_buildOptions);
    }
};

class TargetParser
{
    static inline Target::SourceGroup extract_source(const QJsonValue &source)
    {
        const auto srcObj = source.toObject();
        return {srcObj["language"].toString(),
                srcObj["compiler"].toVariant().toStringList(),
                srcObj["parameters"].toVariant().toStringList(),
                srcObj["sources"].toVariant().toStringList(),
                srcObj["generated_sources"].toVariant().toStringList()};
    }

    static inline Target::SourceGroupList extract_sources(const QJsonArray &sources)
    {
        Target::SourceGroupList res;
        std::transform(std::cbegin(sources),
                       std::cend(sources),
                       std::back_inserter(res),
                       extract_source);
        return res;
    }

    static inline Target extract_target(const QJsonValue &target)
    {
        auto targetObj = target.toObject();
        Target t{targetObj["type"].toString(),
                 targetObj["name"].toString(),
                 targetObj["id"].toString(),
                 targetObj["defined_in"].toString(),
                 targetObj["filename"].toVariant().toStringList(),
                 targetObj["extra_files"].toVariant().toStringList(),
                 targetObj["subproject"].toString(),
                 extract_sources(targetObj["target_sources"].toArray())};
        return t;
    }

    static inline TargetsList load_targets(const QJsonArray &arr)
    {
        TargetsList targets;
        std::transform(std::cbegin(arr),
                       std::cend(arr),
                       std::back_inserter(targets),
                       extract_target);
        return targets;
    }

public:
    static TargetsList targetList(const QJsonDocument &js)
    {
        if (auto obj = get<QJsonArray>(js.object(), "targets"))
            return load_targets(*obj);
        return {};
    }

    static TargetsList targetList(const FilePath &buildDir)
    {
        const FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_TARGETS;
        if (auto arr = load<QJsonArray>(path.toFSPathString()))
            return load_targets(*arr);
        return {};
    }
};

class BuildSystemFilesParser
{
    static void appendFiles(const std::optional<QJsonArray> &arr, FilePaths &dest)
    {
        if (arr)
            std::transform(std::cbegin(*arr),
                           std::cend(*arr),
                           std::back_inserter(dest),
                           [](const auto &file) {
                               return FilePath::fromString(file.toString());
                           });
    }

public:
    static inline FilePaths files(const FilePath &buildDir)
    {
        FilePaths files;
        FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_BUILDSYSTEM_FILES;
        auto arr = load<QJsonArray>(path.toFSPathString());
        appendFiles(arr, files);
        return files;
    }

    static inline FilePaths files(const QJsonDocument &js)
    {
        FilePaths files;
        auto arr = get<QJsonArray>(js.object(), "projectinfo", "buildsystem_files");
        appendFiles(arr, files);
        const auto subprojects = get<QJsonArray>(js.object(), "projectinfo", "subprojects");
        for (const auto &subproject : *subprojects) {
            auto arr = get<QJsonArray>(subproject.toObject(), "buildsystem_files");
            appendFiles(arr, files);
        }
        return files;
    }
};

Result parse(const FilePath &buildDir)
{
    return {TargetParser::targetList(buildDir),
            BuildOptionsParser{buildDir}.takeBuildOptions(),
            BuildSystemFilesParser::files(buildDir)};
}

Result parse(const QByteArray &data)
{
    auto json = QJsonDocument::fromJson(data);
    return {TargetParser::targetList(json),
            BuildOptionsParser{json}.takeBuildOptions(),
            BuildSystemFilesParser::files(json)};
}

Result parse(QIODevice *introFile)
{
    if (introFile) {
        if (!introFile->isOpen())
            introFile->open(QIODevice::ReadOnly | QIODevice::Text);
        introFile->seek(0);
        auto data = introFile->readAll();
        return parse(data);
    }
    return {};
}

} // namespace MesonProjectManager::Internal::MesonInfoParser
