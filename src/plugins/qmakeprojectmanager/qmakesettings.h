// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace QmakeProjectManager {
namespace Internal {

class QmakeSettings : public Utils::AspectContainer
{
    Q_OBJECT

public:
    static QmakeSettings &instance();
    static bool warnAgainstUnalignedBuildDir();
    static bool alwaysRunQmake();
    static bool runSystemFunction();

signals:
    void settingsChanged();

private:
    QmakeSettings();
    friend class SettingsWidget;

    Utils::BoolAspect m_warnAgainstUnalignedBuildDir;
    Utils::BoolAspect m_alwaysRunQmake;
    Utils::BoolAspect m_ignoreSystemFunction;
};

class QmakeSettingsPage final : public Core::IOptionsPage
{
public:
    QmakeSettingsPage();
};

} // namespace Internal
} // namespace QmakeProjectManager
