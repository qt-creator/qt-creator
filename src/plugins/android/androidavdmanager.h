// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidconfigurations.h"

#include <QFuture>

#include <optional>

namespace Android::Internal {

class AndroidAvdManager
{
public:
    AndroidAvdManager(const AndroidConfig& config = AndroidConfigurations::currentConfig());
    ~AndroidAvdManager();

    QFuture<CreateAvdInfo> createAvd(CreateAvdInfo info) const;
    QFuture<AndroidDeviceInfoList> avdList() const;

    QString startAvd(const QString &name) const;
    bool startAvdAsync(const QString &avdName) const;
    QString findAvd(const QString &avdName) const;
    QString waitForAvd(const QString &avdName, const std::optional<QFuture<void>> &future = {}) const;
    bool isAvdBooted(const QString &device) const;
    static bool avdManagerCommand(const AndroidConfig &config,
                                  const QStringList &args,
                                  QString *output);
    const AndroidConfig &config() const { return m_config; }

private:
    bool waitForBooted(const QString &serialNumber, const std::optional<QFuture<void>> &future = {}) const;

private:
    const AndroidConfig &m_config;
};

} // Android::Internal
