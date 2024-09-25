// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QProcessEnvironment>
#include <QString>

namespace QmlDesigner {

class PuppetStartData
{
public:
    QString puppetPath;
    QString workingDirectoryPath;
    QString forwardOutput;
    QString freeTypeOption;
    QString debugPuppet;
    QProcessEnvironment environment;
};
} // namespace QmlDesigner
