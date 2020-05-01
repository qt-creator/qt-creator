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
#include "../mesoninfo.h"
#include "./common.h"
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
