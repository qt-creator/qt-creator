/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/settingsaccessor.h>

#include <coreplugin/id.h>

#include <QList>

#include <memory>

namespace Core { class Id; }

namespace CMakeProjectManager {

class CMakeTool;

namespace Internal {

class CMakeToolSettingsAccessor : public Utils::UpgradingSettingsAccessor
{
public:
    CMakeToolSettingsAccessor();

    struct CMakeTools {
        Core::Id defaultToolId;
        std::vector<std::unique_ptr<CMakeTool>> cmakeTools;
    };

    CMakeTools restoreCMakeTools(QWidget *parent) const;

    void saveCMakeTools(const QList<CMakeTool *> &cmakeTools,
                        const Core::Id &defaultId,
                        QWidget *parent);

private:
    CMakeTools cmakeTools(const QVariantMap &data, bool fromSdk) const;
};

} // namespace Internal
} // namespace CMakeProjectManager
