// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace Coco::Internal {

class CocoSettings final : public Utils::AspectContainer
{
    Q_OBJECT

    friend CocoSettings &cocoSettings();
    CocoSettings();

public:
    Utils::FilePath coverageBrowserPath() const;

    bool isValid() const;
    QString errorMessage() const;

    void apply() final;

    Utils::FilePathAspect cocoPath{this};

signals:
    void updateCocoDir();

private:
    void setDirectoryVars(const Utils::FilePath &dir);
    void findDefaultDirectory();

    void logError(const QString &msg);
    bool isCocoDirectory(const Utils::FilePath &cocoDir) const;
    bool verifyCocoDirectory(const Utils::FilePath &cocoDir);
    void tryPath(const QString &path);

    bool m_isValid = false;
    QString m_errorMessage;
};

CocoSettings &cocoSettings();

void setupCocoSettings();

} // namespace Coco::Internal
