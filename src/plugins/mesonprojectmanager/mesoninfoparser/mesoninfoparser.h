// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
