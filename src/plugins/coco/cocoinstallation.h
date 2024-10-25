// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

class QString;

namespace Coco::Internal {

struct CocoInstallationPrivate;

// Borg pattern: There are many instances of this class, but all are the same.
class CocoInstallation
{
public:
    CocoInstallation();

    Utils::FilePath directory() const;
    Utils::FilePath coverageBrowserPath() const;
    void setDirectory(const Utils::FilePath &dir);
    void findDefaultDirectory();

    bool isValid() const;
    QString errorMessage() const;

private:
    Utils::FilePath coverageScannerPath(const Utils::FilePath &cocoDir) const;
    void logError(const QString &msg);
    bool isCocoDirectory(const Utils::FilePath &cocoDir) const;
    bool verifyCocoDirectory();
    void tryPath(const QString &path);
    QString envVar(const QString &var) const;

    static CocoInstallationPrivate *d;
};

} // namespace Coco::Internal
