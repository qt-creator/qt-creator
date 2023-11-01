// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "common.h"
#include "mesonpluginconstants.h"

#include <utils/fileutils.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace MesonProjectManager {
namespace Internal {

class BuildSystemFilesParser
{
    std::vector<Utils::FilePath> m_files;
    static void appendFiles(const std::optional<QJsonArray> &arr, std::vector<Utils::FilePath> &dest)
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
    BuildSystemFilesParser(const Utils::FilePath &buildDir)
    {
        Utils::FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_BUILDSYSTEM_FILES;
        auto arr = load<QJsonArray>(path.toFSPathString());
        appendFiles(arr, m_files);
    }

    BuildSystemFilesParser(const QJsonDocument &js)
    {
        auto arr = get<QJsonArray>(js.object(), "projectinfo", "buildsystem_files");
        appendFiles(arr, m_files);
        const auto subprojects = get<QJsonArray>(js.object(), "projectinfo", "subprojects");
        for (const auto &subproject : *subprojects) {
            auto arr = get<QJsonArray>(subproject.toObject(), "buildsystem_files");
            appendFiles(arr, m_files);
        }
    }

    std::vector<Utils::FilePath> files() { return m_files; };
};

} // namespace Internal
} // namespace MesonProjectManager
