// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonpluginconstants.h"
#include "target.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace MesonProjectManager {
namespace Internal {

class TargetParser
{
    TargetsList m_targets;
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
    TargetParser(const Utils::FilePath &buildDir)
    {
        Utils::FilePath path = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INTRO_TARGETS;
        auto arr = load<QJsonArray>(path.toFSPathString());
        if (arr)
            m_targets = load_targets(*arr);
    }

    TargetParser(const QJsonDocument &js)
    {
        auto obj = get<QJsonArray>(js.object(), "targets");
        if (obj)
            m_targets = load_targets(*obj);
    }

    inline TargetsList targetList() { return m_targets; }
};

} // namespace Internal
} // namespace MesonProjectManager
