// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "common.h"
#include "mesoninfo.h"
#include "mesonpluginconstants.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace MesonProjectManager {
namespace Internal {

class InfoParser
{
    static inline MesonInfo load_info(const QJsonObject &obj)
    {
        MesonInfo info;
        auto version = obj["meson_version"].toObject();
        info.mesonVersion = Version{version["major"].toInt(),
                                    version["minor"].toInt(),
                                    version["patch"].toInt()};
        return info;
    }
    MesonInfo m_info;

public:
    InfoParser(const QString &buildDir)
    /*: AbstractParser(jsonFile(buildDir))*/
    {
        auto obj = load<QJsonObject>(jsonFile(buildDir));
        if (obj)
            m_info = load_info(*obj);
    }
    static inline QString jsonFile(const QString &buildDir)
    {
        return QString("%1/%2/%3")
            .arg(buildDir)
            .arg(Constants::MESON_INFO_DIR)
            .arg(Constants::MESON_INFO);
    }
    MesonInfo info() { return m_info; }
};

} // namespace Internal
} // namespace MesonProjectManager
