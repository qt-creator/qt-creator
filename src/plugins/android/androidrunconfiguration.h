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

#include "adbcommandswidget.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

namespace Android {

class BaseStringListAspect : public ProjectExplorer::ProjectConfigurationAspect
{
    Q_OBJECT

public:
    explicit BaseStringListAspect(const QString &settingsKey = QString(),
                                  Utils::Id id = Utils::Id());
    ~BaseStringListAspect() override;

    void addToLayout(ProjectExplorer::LayoutBuilder &builder) override;

    QStringList value() const;
    void setValue(const QStringList &val);

    void setLabel(const QString &label);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

signals:
    void changed();

private:
    QStringList m_value;
    QString m_label;
    QPointer<Android::Internal::AdbCommandsWidget> m_widget; // Owned by RunConfigWidget
};

class ANDROID_EXPORT AndroidRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    explicit AndroidRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);
};

} // namespace Android
