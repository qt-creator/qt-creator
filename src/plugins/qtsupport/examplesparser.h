// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/welcomepagehelper.h>
#include <utils/expected.h>
#include <utils/filepath.h>

namespace QtSupport::Internal {

enum InstructionalType { Example = 0, Demo, Tutorial };

class ExampleItem : public Core::ListItem
{
public:
    Utils::FilePath projectPath;
    QString docUrl;
    Utils::FilePaths filesToOpen;
    Utils::FilePath mainFile; /* file to be visible after opening filesToOpen */
    Utils::FilePaths dependencies;
    InstructionalType type;
    int difficulty = 0;
    bool hasSourceCode = false;
    bool isVideo = false;
    bool isHighlighted = false;
    QString videoUrl;
    QString videoLength;
    QStringList platforms;
};

Utils::expected_str<QList<ExampleItem *>> parseExamples(const Utils::FilePath &manifest,
                                                        const Utils::FilePath &examplesInstallPath,
                                                        const Utils::FilePath &demosInstallPath,
                                                        bool examples);

} // namespace QtSupport::Internal

Q_DECLARE_METATYPE(QtSupport::Internal::ExampleItem *)
