// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QProcessEnvironment>

namespace QmlDesigner {

class PuppetStartData
{
public:
    Utils::FilePath puppetPath;
    Utils::FilePath workingDirectoryPath;
    QString forwardOutput;
    QString freeTypeOption;
    QString debugPuppet;
    QProcessEnvironment environment;
};

} // namespace QmlDesigner
