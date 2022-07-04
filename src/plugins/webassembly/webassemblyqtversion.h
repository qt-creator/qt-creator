// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/qtversionfactory.h>
#include <qtsupport/baseqtversion.h>

namespace WebAssembly {
namespace Internal {

class WebAssemblyQtVersion : public QtSupport::QtVersion
{
    Q_DECLARE_TR_FUNCTIONS(WebAssembly::Internal::WebAssemblyQtVersion)

public:
    WebAssemblyQtVersion();

    QString description() const override;

    QSet<Utils::Id> targetDeviceTypes() const override;

    bool isValid() const override;
    QString invalidReason() const override;

    static const QVersionNumber &minimumSupportedQtVersion();
    static bool isQtVersionInstalled();
    static bool isUnsupportedQtVersionInstalled();
};

class WebAssemblyQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    WebAssemblyQtVersionFactory();
};

} // namespace Internal
} // namespace WebAssembly
