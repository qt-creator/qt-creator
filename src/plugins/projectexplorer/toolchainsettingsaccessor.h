// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>
#include <utils/settingsaccessor.h>
#include <utils/store.h>

#include <QList>

namespace ProjectExplorer {

class Toolchain;

namespace Internal {

class ToolchainSettingsAccessor : public Utils::UpgradingSettingsAccessor
{
public:
    ToolchainSettingsAccessor();

    QList<Toolchain *> restoreToolchains();
    QList<Toolchain *> retryDeferredToolchains();
    bool isDeferred(Utils::Id toolchainId) const;

#ifdef WITH_TESTS
    void deferForTest(Utils::Id toolchainId);
    void removeDeferredForTest(Utils::Id toolchainId);
#endif

    void saveToolchains(const QList<Toolchain *> &toolchains);

private:
    QList<Toolchain *> toolChains(const Utils::Store &data, QList<Utils::Store> &deferred) const;

    // Raw data of stored toolchains whose factory is not registered yet, kept for a later retry.
    QList<Utils::Store> m_deferredSdkToolchains;
    QList<Utils::Store> m_deferredUserToolchains;
};

QObject *createToolchainSettingsTest();

} // namespace Internal
} // namespace ProjectExplorer
