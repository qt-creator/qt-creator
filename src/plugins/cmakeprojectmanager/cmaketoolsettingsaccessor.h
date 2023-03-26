// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>
#include <utils/settingsaccessor.h>

namespace CMakeProjectManager {

class CMakeTool;

namespace Internal {

class CMakeToolSettingsAccessor : public Utils::UpgradingSettingsAccessor
{
public:
    CMakeToolSettingsAccessor();

    struct CMakeTools {
        Utils::Id defaultToolId;
        std::vector<std::unique_ptr<CMakeTool>> cmakeTools;
    };

    CMakeTools restoreCMakeTools(QWidget *parent) const;

    void saveCMakeTools(const QList<CMakeTool *> &cmakeTools,
                        const Utils::Id &defaultId,
                        QWidget *parent);

private:
    CMakeTools cmakeTools(const QVariantMap &data, bool fromSdk) const;
};

} // namespace Internal
} // namespace CMakeProjectManager
