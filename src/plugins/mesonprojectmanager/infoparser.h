// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "common.h"
#include "mesonpluginconstants.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVersionNumber>

namespace MesonProjectManager::Internal {

class InfoParser
{
    static inline QVersionNumber load_info(const QJsonObject &obj)
    {
        auto version = obj["meson_version"].toObject();
        return {version["major"].toInt(), version["minor"].toInt(), version["patch"].toInt()};
    }
    QVersionNumber m_info;

public:
    InfoParser(const Utils::FilePath &buildDir)
    {
        Utils::FilePath jsonFile = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INFO;
        auto obj = load<QJsonObject>(jsonFile.toFSPathString());
        if (obj)
            m_info = load_info(*obj);
    }

    QVersionNumber info() { return m_info; }
};

} // namespace MesonProjectManager::Internal
