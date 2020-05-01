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
#include "./common.h"
#include "utils/fileutils.h"
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
    static void appendFiles(const Utils::optional<QJsonArray> &arr,
                            std::vector<Utils::FilePath> &dest)
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
    BuildSystemFilesParser(const QString &buildDir)
    {
        auto arr = load<QJsonArray>(QString("%1/%2/%3")
                                        .arg(buildDir)
                                        .arg(Constants::MESON_INFO_DIR)
                                        .arg(Constants::MESON_INTRO_BUILDSYSTEM_FILES));
        appendFiles(arr, m_files);
    }

    BuildSystemFilesParser(const QJsonDocument &js)
    {
        auto arr = get<QJsonArray>(js.object(), "projectinfo", "buildsystem_files");
        appendFiles(arr, m_files);
        auto subprojects = get<QJsonArray>(js.object(), "projectinfo", "subprojects");
        std::for_each(std::cbegin(*subprojects),
                      std::cend(*subprojects),
                      [this](const auto &subproject) {
                          auto arr = get<QJsonArray>(subproject.toObject(), "buildsystem_files");
                          appendFiles(arr, m_files);
                      });
    }

    std::vector<Utils::FilePath> files() { return m_files; };
};
} // namespace Internal
} // namespace MesonProjectManager
