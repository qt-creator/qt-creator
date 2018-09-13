/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "projectexplorer_export.h"

#include "runconfiguration.h"

#include <utils/environment.h>

#include <QList>
#include <QVariantMap>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT EnvironmentAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    // The environment the user chose as base for his modifications.
    virtual Utils::Environment baseEnvironment() const = 0;
    // The environment including the user's modifications.
    Utils::Environment environment() const;

    QList<int> possibleBaseEnvironments() const;
    QString baseEnvironmentDisplayName(int base) const;

    int baseEnvironmentBase() const;
    void setBaseEnvironmentBase(int base);

    QList<Utils::EnvironmentItem> userEnvironmentChanges() const { return m_changes; }
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);

    void addSupportedBaseEnvironment(int base, const QString &displayName);
    void addPreferredBaseEnvironment(int base, const QString &displayName);

signals:
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);
    void environmentChanged();

protected:
    EnvironmentAspect();
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    int m_base = -1;
    QList<Utils::EnvironmentItem> m_changes;
    QMap<int, QString> m_displayNames;
};

} // namespace ProjectExplorer
