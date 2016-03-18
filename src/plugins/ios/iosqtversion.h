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

#include <qtsupport/baseqtversion.h>

#include <QCoreApplication>

namespace Ios {
namespace Internal {

class IosQtVersion : public QtSupport::BaseQtVersion
{
    Q_DECLARE_TR_FUNCTIONS(Ios::Internal::IosQtVersion)

public:
    IosQtVersion();
    IosQtVersion(const Utils::FileName &path, bool isAutodetected = false,
                 const QString &autodetectionSource = QString());

    IosQtVersion *clone() const override;
    QString type() const override;
    bool isValid() const override;
    QString invalidReason() const override;

    QList<ProjectExplorer::Abi> detectQtAbis() const override;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;

    QSet<Core::Id> availableFeatures() const override;
    QSet<Core::Id> targetDeviceTypes() const override;

    QString description() const override;
};

} // namespace Internal
} // namespace Ios
