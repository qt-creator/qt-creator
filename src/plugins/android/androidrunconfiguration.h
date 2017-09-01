/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "android_global.h"

#include <projectexplorer/runconfiguration.h>

namespace Android {

class ANDROID_EXPORT AndroidRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    explicit AndroidRunConfiguration(ProjectExplorer::Target *target);

    QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    const QStringList &amStartExtraArgs() const;
    const QStringList &preStartShellCommands() const;
    const QStringList &postFinishShellCommands() const;

private:
    // FIXME: This appears to miss a copyFrom() implementation.
    void setPreStartShellCommands(const QStringList &cmdList);
    void setPostFinishShellCommands(const QStringList &cmdList);
    void setAmStartExtraArgs(const QStringList &args);

    QStringList m_amStartExtraArgs;
    QStringList m_preStartShellCommands;
    QStringList m_postFinishShellCommands;
};

} // namespace Android
