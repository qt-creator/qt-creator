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
#include "buildoptions.h"
#include "mesoninfo.h"
#include "parsers/buildoptionsparser.h"
#include "parsers/buildsystemfilesparser.h"
#include "parsers/infoparser.h"
#include "parsers/targetparser.h"
#include "target.h"
#include <utils/fileutils.h>
#include <utils/optional.h>
#include <QString>
#include <QVariant>
namespace MesonProjectManager {
namespace Internal {
class MesonInfoParserPrivate;

namespace MesonInfoParser {
struct Result
{
    TargetsList targets;
    BuildOptionsList buildOptions;
    std::vector<Utils::FilePath> buildSystemFiles;
    Utils::optional<MesonInfo> mesonInfo;
};

inline Result parse(const QString &buildDir)
{
    return {TargetParser{buildDir}.targetList(),
            BuildOptionsParser{buildDir}.takeBuildOptions(),
            BuildSystemFilesParser{buildDir}.files(),
            InfoParser{buildDir}.info()};
}

inline Result parse(const QByteArray &data)
{
    auto json = QJsonDocument::fromJson(data);
    return {TargetParser{json}.targetList(),
            BuildOptionsParser{json}.takeBuildOptions(),
            BuildSystemFilesParser{json}.files(),
            Utils::nullopt};
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
inline Utils::optional<MesonInfo> mesonInfo(const QString &buildDir)
{
    return InfoParser{buildDir}.info();
}

} // namespace MesonInfoParser
} // namespace Internal
} // namespace MesonProjectManager
