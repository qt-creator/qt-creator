// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/welcomepagehelper.h>
#include <utils/expected.h>

namespace QtSupport::Internal {

enum InstructionalType { Example = 0, Demo, Tutorial };

class ExampleItem : public Core::ListItem
{
public:
    QString projectPath;
    QString docUrl;
    QStringList filesToOpen;
    QString mainFile; /* file to be visible after opening filesToOpen */
    QStringList dependencies;
    InstructionalType type;
    int difficulty = 0;
    bool hasSourceCode = false;
    bool isVideo = false;
    bool isHighlighted = false;
    QString videoUrl;
    QString videoLength;
    QStringList platforms;
};

Utils::expected_str<QList<ExampleItem *>> parseExamples(const QString &manifest,
                                                        const QString &examplesInstallPath,
                                                        const QString &demosInstallPath,
                                                        bool examples);

} // namespace QtSupport::Internal

Q_DECLARE_METATYPE(QtSupport::Internal::ExampleItem *)
