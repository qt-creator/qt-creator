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
#include "../target.h"
#include "./common.h"
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
    TargetParser(const QString &buildDir)
    {
        auto arr = load<QJsonArray>(QString("%1/%2/%3")
                                        .arg(buildDir)
                                        .arg(Constants::MESON_INFO_DIR)
                                        .arg(Constants::MESON_INTRO_TARGETS));
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
