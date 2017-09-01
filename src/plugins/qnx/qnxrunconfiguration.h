/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include <remotelinux/remotelinuxrunconfiguration.h>

namespace Qnx {
namespace Internal {

class QnxRunConfiguration : public RemoteLinux::RemoteLinuxRunConfiguration
{
    Q_OBJECT

public:
    explicit QnxRunConfiguration(ProjectExplorer::Target *target);

    ProjectExplorer::Runnable runnable() const override;

    QWidget *createConfigurationWidget() override;
    QVariantMap toMap() const override;

private:
    friend class ProjectExplorer::IRunConfigurationFactory;

    void copyFrom(const QnxRunConfiguration *source);
    void initialize(Core::Id id, const QString &targetName);

    bool fromMap(const QVariantMap &map) override;

    QString m_qtLibPath;
};

} // namespace Internal
} // namespace Qnx
