// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Haskell {
namespace Internal {

class HaskellManager : public QObject
{
    Q_OBJECT

public:
    static HaskellManager *instance();

    static Utils::FilePath findProjectDirectory(const Utils::FilePath &filePath);
    static Utils::FilePath stackExecutable();
    static void setStackExecutable(const Utils::FilePath &filePath);
    static void openGhci(const Utils::FilePath &haskellFile);
    static void readSettings(QSettings *settings);
    static void writeSettings(QSettings *settings);

signals:
    void stackExecutableChanged(const Utils::FilePath &filePath);
};

} // namespace Internal
} // namespace Haskell
