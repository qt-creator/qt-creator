/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#pragma once
#include "../../mesonpluginconstants.h"
#include "../buildoptions.h"
#include "./common.h"

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
    BuildOptionsParser(const QString &buildDir)
    {
        auto arr = load<QJsonArray>(QString("%1/%2/%3")
                                        .arg(buildDir)
                                        .arg(Constants::MESON_INFO_DIR)
                                        .arg(Constants::MESON_INTRO_BUIDOPTIONS));
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
