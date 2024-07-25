// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildoptions.h"
#include "buildoptionsparser.h"
#include "infoparser.h"
#include "target.h"

#include <utils/filepath.h>

#include <optional>

namespace MesonProjectManager {
namespace Internal {
namespace MesonInfoParser {

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

    static TargetsList targetList(const Utils::FilePath &buildDir)
    {
        Utils::FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_TARGETS;
        if (auto arr = load<QJsonArray>(path.toFSPathString()))
            return load_targets(*arr);
        return {};
    }
};

class BuildSystemFilesParser
{
    static void appendFiles(const std::optional<QJsonArray> &arr, Utils::FilePaths &dest)
    {
        if (arr)
            std::transform(std::cbegin(*arr),
                           std::cend(*arr),
                           std::back_inserter(dest),
                           [](const auto &file) {
                               return Utils::FilePath::fromString(file.toString());
                           });
    }

public:
    static inline Utils::FilePaths files(const Utils::FilePath &buildDir)
    {
        Utils::FilePaths files;
        Utils::FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_BUILDSYSTEM_FILES;
        auto arr = load<QJsonArray>(path.toFSPathString());
        appendFiles(arr, files);
        return files;
    }

    static inline Utils::FilePaths files(const QJsonDocument &js)
    {
        Utils::FilePaths files;
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


struct Result
{
    TargetsList targets;
    BuildOptionsList buildOptions;
    Utils::FilePaths buildSystemFiles;
    std::optional<QVersionNumber> mesonInfo;
};

inline Result parse(const Utils::FilePath &buildDir)
{
    return {TargetParser::targetList(buildDir),
            BuildOptionsParser{buildDir}.takeBuildOptions(),
            BuildSystemFilesParser::files(buildDir),
            InfoParser{buildDir}.info()};
}

inline Result parse(const QByteArray &data)
{
    auto json = QJsonDocument::fromJson(data);
    return {TargetParser::targetList(json),
            BuildOptionsParser{json}.takeBuildOptions(),
            BuildSystemFilesParser::files(json),
            std::nullopt};
}

inline Result parse(QIODevice *introFile)
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

inline std::optional<QVersionNumber> mesonInfo(const Utils::FilePath &buildDir)
{
    return InfoParser{buildDir}.info();
}

} // namespace MesonInfoParser
} // namespace Internal
} // namespace MesonProjectManager
