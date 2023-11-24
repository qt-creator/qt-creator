// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>

namespace WebAssembly::Internal {

class WebAssemblyQtVersion final : public QtSupport::QtVersion
{
public:
    WebAssemblyQtVersion();

    QString description() const final;

    QSet<Utils::Id> targetDeviceTypes() const final;

    bool isValid() const final;
    QString invalidReason() const final;

    static const QVersionNumber &minimumSupportedQtVersion();
    static bool isQtVersionInstalled();
    static bool isUnsupportedQtVersionInstalled();
};

void setupWebAssemblyQtVersion();

} // WebAssembly::Internal
