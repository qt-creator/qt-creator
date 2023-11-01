// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildoptions.h"
#include "buildoptionsparser.h"
#include "buildsystemfilesparser.h"
#include "infoparser.h"
#include "mesoninfo.h"
#include "target.h"
#include "targetparser.h"

#include <utils/filepath.h>

#include <optional>

namespace MesonProjectManager {
namespace Internal {

class MesonInfoParserPrivate;

namespace MesonInfoParser {

struct Result
{
    TargetsList targets;
    BuildOptionsList buildOptions;
    std::vector<Utils::FilePath> buildSystemFiles;
    std::optional<MesonInfo> mesonInfo;
};

inline Result parse(const Utils::FilePath &buildDir)
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
            std::nullopt};
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

inline std::optional<MesonInfo> mesonInfo(const Utils::FilePath &buildDir)
{
    return InfoParser{buildDir}.info();
}

} // namespace MesonInfoParser
} // namespace Internal
} // namespace MesonProjectManager
