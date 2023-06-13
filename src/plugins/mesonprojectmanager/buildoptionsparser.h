// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildoptions.h"
#include "common.h"
#include "mesonpluginconstants.h"

#include <utils/filepath.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <memory>

namespace MesonProjectManager {
namespace Internal {

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
    BuildOptionsParser(const Utils::FilePath &buildDir)
    {
        Utils::FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_BUIDOPTIONS;
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

} // namespace Internal
} // namespace MesonProjectManager
