/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "iosconfigurations.h"
#include "simulatorcontrol.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace Ios {
namespace Internal {

namespace Ui { class IosSettingsWidget; }

class IosSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Ios::Internal::IosSettingsWidget)

public:
    IosSettingsWidget();
    ~IosSettingsWidget() final;

private:
    void apply() final;

    void saveSettings();

    void onStart();
    void onCreate();
    void onReset();
    void onRename();
    void onDelete();
    void onScreenshot();
    void onSelectionChanged();

private:
    Ui::IosSettingsWidget *m_ui = nullptr;
    SimulatorControl *m_simControl = nullptr;
};

} // namespace Internal
} // namespace Ios
