// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionfactory.h>

#include <QCoreApplication>

namespace Ios {
namespace Internal {

class IosQtVersion : public QtSupport::QtVersion
{
    Q_DECLARE_TR_FUNCTIONS(Ios::Internal::IosQtVersion)

public:
    IosQtVersion();

    bool isValid() const override;
    QString invalidReason() const override;

    ProjectExplorer::Abis detectQtAbis() const override;

    QSet<Utils::Id> availableFeatures() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;

    QString description() const override;
};

class IosQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    IosQtVersionFactory();
};

} // namespace Internal
} // namespace Ios
